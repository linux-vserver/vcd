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
#include <vserver.h>
#include <sys/wait.h>

#include "xmlrpc.h"

#include "auth.h"
#include "cfg.h"
#include "log.h"
#include "methods.h"
#include "vxdb.h"

/* stop process:
   1) create new vps killer (background)
   2) run init-style based stop command
   3) wait for vps killer
*/

static xid_t xid = 0;
static char *name = NULL;
static char *errmsg = NULL;

static
int init_exec(char *method)
{
	pid_t pid;
	int i, status, waitchild = 1;
	
	struct vx_flags cflags = {
		.flags = 0,
		.mask = 0,
	};
	
	struct vx_migrate_flags migrate_flags = {
		.flags = 0,
	};
	
	char *vdirbase = cfg_getstr(cfg, "vserver-basedir");
	char *vdir = NULL;
	
	asprintf(&vdir, "%s/%s", vdirbase, name);
	
	switch ((pid = fork())) {
	case -1:
		return method_error(&errmsg, "fork: %s", strerror(errno));
	
	case 0:
		for (i = 0; i < 100; i++)
			close(i);
		
		if (vx_enter_namespace(xid) == -1 ||
		    chroot(vdir) == -1 ||
		    nx_migrate(xid) == -1 ||
		    vx_migrate(xid, &migrate_flags) == -1)
			exit(EXIT_FAILURE);
		
		if (strcmp(method, "init") == 0)
			execl("/sbin/telinit", "/sbin/telinit", "0", NULL);
		else
			exit(EXIT_FAILURE);
		
		exit(EXIT_FAILURE);
	
	default:
		/* to wait or not to wait!? */
		switch (waitpid(pid, &status, waitchild == 0 ? WNOHANG : 0)) {
		case -1:
			return method_error(&errmsg, "waitpid: %s", strerror(errno));
		
		case 0:
			break;
		
		default:
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				return method_error(&errmsg, "init command failed");
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
		}
	}
	
	return 0;
}

static
void stop_timeout_handler(int sig)
{
	int i;
	
	struct vx_kill_opts kill_opts = {
		.pid = 0,
		.sig = 9,
	};
	
	signal(SIGALRM, SIG_DFL);
	
	for (i = 0; i < 5; i++) {
		vx_kill(xid, &kill_opts);
		sleep(1);
		
		if (vx_get_info(xid, NULL) == -1 && errno == ESRCH)
			break;
	}
	
	exit(EXIT_FAILURE);
}

/* vx.stop(string name) */
XMLRPC_VALUE m_vx_stop(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	XMLRPC_VALUE response;
	char *method, *stop;
	int timeout;
	
	struct vx_kill_opts kill_opts = {
		.pid = 1,
		.sig = SIGTERM,
	};
	
	if (!auth_isadmin(r) && !auth_isowner(r))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	name = XMLRPC_VectorGetStringWithID(params, "name");
	
	if (!name)
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	if (vxdb_getxid(name, &xid) == -1)
		return XMLRPC_UtilityCreateFault(404, "Not Found");
	
	if (vx_get_info(xid, NULL) == -1)
		return XMLRPC_UtilityCreateFault(404, "Not Running");
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT method,stop,timeout FROM init_method WHERE xid = %d",
		xid);
	
	if (!dbr || dbi_result_get_numrows(dbr) < 1) {
		method = "init";
		stop = "";
		timeout = 10;
	}
	
	else {
		dbi_result_first_row(dbr);
		method  = (char *) dbi_result_get_string(dbr, "method");
		stop    = (char *) dbi_result_get_string(dbr, "stop");
		timeout =          dbi_result_get_int(dbr, "timeout");
	}
	
	if (init_exec(method) == -1)
		return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	
	signal(SIGALRM, stop_timeout_handler);
	alarm(timeout + 10);
	
	if (vx_wait(xid, NULL) == -1 && errno != ESRCH)
		return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	
	return params;
}
