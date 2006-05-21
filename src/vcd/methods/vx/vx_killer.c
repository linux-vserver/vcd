// Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "lucid.h"
#include "xmlrpc.h"

#include "auth.h"
#include "cfg.h"
#include "log.h"
#include "methods.h"
#include "vxdb.h"

/* killer process:
   1) fork to background if appropriate
   2) shutdown init less contexts
   3) wait for context to die using a timeout
   4) if timeout is reached kill all processes in context
   5) restart OR exit if all processes in context died
*/

static XMLRPC_SERVER _server;
static XMLRPC_REQUEST _request;

static char *name = NULL;
static xid_t xid = 0;
static int reboot = 0;

static
void handle_death(void)
{
	log_debug("vx killer success for '%s'", name);
	
	if (reboot == 0)
		exit(EXIT_SUCCESS);
	
	signal(SIGCHLD, SIG_DFL);
	
	log_debug("vx killer restarting '%s'", name);
	XMLRPC_VALUE response = m_vx_start(_server, _request, NULL);
	
	char *fault_string = XMLRPC_VectorGetStringWithID(response, "faultString");
	int   fault_code   = XMLRPC_VectorGetIntWithID(response, "faultCode");
	
	if (fault_string || fault_code != 0) {
		log_warn("vx killer restart failed: %s (%d)", fault_string, fault_code);
		exit(EXIT_FAILURE);
	}
	
	exit(EXIT_SUCCESS);
}

static
void wait_timeout_handler(int sig)
{
	int i;
	
	struct vx_kill_opts kill_opts = {
		.pid = 0,
		.sig = 9,
	};
	
	log_debug("vx killer reached timeout before death");
	
	signal(SIGALRM, SIG_DFL);
	
	for (i = 0; i < 5; i++) {
		log_debug("vx killer terminates all processes (%d)", i);
		vx_kill(xid, &kill_opts);
		sleep(1);
		
		if (vx_get_info(xid, NULL) == -1 && errno == ESRCH)
			break;
	}
	
	handle_death();
}

static
void wait_death(int timeout)
{
	signal(SIGALRM, wait_timeout_handler);
	alarm(timeout);
	log_debug("vx killer now waiting %d seconds for context death", timeout);
	
	if (vx_wait(xid, NULL) == -1 && errno != ESRCH)
		log_debug("vx killer could not wait: %s", strerror(errno));
	
	log_debug("vx killer didn't reach timeout before death");
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	
	handle_death();
}

static
int initless_shutdown(void)
{
	dbi_result dbr;
	char *method, *stop;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT method,stop FROM init_method WHERE xid = %d",
		xid);
	
	if (!dbr)
		return errno = MEVXDB, -1;
	 
	if (dbi_result_get_numrows(dbr) < 1)
		return 0;
	
	dbi_result_first_row(dbr);
	method  = (char *) dbi_result_get_string(dbr, "method");
	stop    = (char *) dbi_result_get_string(dbr, "stop");
	
	pid_t pid;
	int i, status;
	
	char *vdirbase = cfg_getstr(cfg, "vserver-basedir");
	char *vdir = NULL;
	
	asprintf(&vdir, "%s/%s", vdirbase, name);
	
	signal(SIGCHLD, SIG_IGN);
	
	switch ((pid = fork())) {
	case -1:
		return errno = MESYS, -1;
	
	case 0:
		signal(SIGCHLD, SIG_DFL);
		usleep(200);
		
		for (i = 0; i < 100; i++)
			close(i);
		
		if (vx_enter_namespace(xid) == -1 ||
		    chroot(vdir) == -1 ||
		    nx_migrate(xid) == -1 ||
		    vx_migrate(xid, NULL) == -1)
			exit(EXIT_FAILURE);
		
		if (strcmp(method, "gentoo") == 0) {
			if (!stop || !*stop)
				stop = "shutdown";
			
			if (exec_replace("/sbin/rc %s", stop) == -1)
				exit(EXIT_FAILURE);
		}
		
		exit(EXIT_SUCCESS);
	
	default:
		break;
	}
	
	return 0;
}

/* vx.killer(string name[, int wait[, int reboot]]) */
XMLRPC_VALUE m_vx_killer(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r) && !auth_isowner(r))
		return method_error(MEPERM);
	
	_server = s;
	_request = r;
	
	name     = XMLRPC_VectorGetStringWithID(params, "name");
	reboot   = XMLRPC_VectorGetIntWithID(params, "reboot");
	int wait = XMLRPC_VectorGetIntWithID(params, "wait");
	
	if (!name)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	if (vx_get_info(xid, NULL) == -1 && errno == ESRCH)
		return method_error(MESTOPPED);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT timeout FROM init_method WHERE xid = %d",
		xid);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	int timeout = 0;
	
	if (dbi_result_get_numrows(dbr) > 0) {
		dbi_result_first_row(dbr);
		timeout = dbi_result_get_int(dbr, "timeout");
	}
	
	if (timeout < 3)
		timeout = 10;
	
	pid_t pid;
	int status;
	
	if (wait)
		signal(SIGCHLD, SIG_DFL);
	else
		signal(SIGCHLD, SIG_IGN);
	
	switch((pid = fork())) {
	case -1:
		return method_error(MESYS);
		break;
	
	case 0:
		log_debug("vx killer activated for '%s'", name);
		
		if (initless_shutdown() == -1)
			exit(EXIT_FAILURE);
		
		wait_death(timeout);
		break;
	
	default:
		if (wait) {
			if (waitpid(pid, &status, 0) == -1)
				return method_error(MESYS);
			
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				return method_error(MESYS);
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
		}
	}
	
	return params;
}
