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

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "lucid.h"

#include "auth.h"
#include "cfg.h"
#include "log.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* killer process:
   1) fork to background if appropriate
   2) shutdown init less contexts
   3) wait for context to die using a timeout
   4) if timeout is reached kill all processes in context
   5) restart OR exit if all processes in context died
*/

static xmlrpc_value *_params_orig = NULL;
static const char *name = NULL;
static xid_t xid = 0;
static int reboot = 0;

static
void handle_death(void)
{
	xmlrpc_env env;
	log_debug("vx killer success for '%s'", name);
	
	if (reboot == 0)
		exit(EXIT_SUCCESS);
	
	signal(SIGCHLD, SIG_DFL);
	
	log_debug("vx killer restarting '%s'", name);
	
	xmlrpc_env_init(&env);
	m_vx_start(&env, _params_orig, NULL);
	
	if (env.fault_occurred) {
		log_warn("vx killer restart failed: %s (%d)", env.fault_string, env.fault_code);
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
	log_debug("vx killer now waiting %d seconds for death (xid = %d)", timeout, xid);
	
	if (vx_wait(xid, NULL) == -1 && errno != ESRCH)
		log_debug("vx killer could not wait: %s", strerror(errno));
	
	log_debug("vx killer didn't reach timeout before death");
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	
	handle_death();
}

static
xmlrpc_value *shutdown_init(xmlrpc_env *env)
{
	return NULL;
}

static
xmlrpc_value *shutdown_initng(xmlrpc_env *env)
{
	return NULL;
}

static
xmlrpc_value *shutdown_sysvrc(xmlrpc_env *env)
{
	return NULL;
}

static
xmlrpc_value *shutdown_gentoo(xmlrpc_env *env, const char *runlevel)
{
	pid_t pid;
	int i, status;
	
	char *vserverdir = cfg_getstr(cfg, "vserverdir");
	char *vdir = NULL;
	
	if (str_isempty(runlevel))
		runlevel = "shutdown";
	
	asprintf(&vdir, "%s/%s", vserverdir, name);
	
	switch ((pid = fork())) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
	
	case 0:
		for (i = 0; i < 100; i++)
			close(i);
		
		if (vx_enter_namespace(xid) == -1)
			exit(errno);
		
		if (chroot_secure_chdir(vdir, "/") == -1)
			exit(errno);
		
		if (chroot(".") == -1)
			exit(errno);
		
		if (nx_migrate(xid) == -1 || vx_migrate(xid, NULL) == -1)
			exit(errno);
		
		if (exec_replace("/sbin/rc %s", runlevel) == -1)
			exit(errno);
		
		/* never get here */
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, WNOHANG) == -1)
			method_return_faultf(env, MESYS, "%s: waitpid: %s", __FUNCTION__, strerror(errno));
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			method_return_faultf(env, MESYS, "shutdown failed: %s", strerror(WEXITSTATUS(status)));
		
		if (WIFSIGNALED(status))
			kill(getpid(), WTERMSIG(status));
	}
	
	return NULL;
}

/* vx.killer(string name[, int wait[, int reboot]]) */
xmlrpc_value *m_vx_killer(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	const char *method, *stop;
	int wait, timeout, status;
	dbi_result dbr;
	pid_t pid;
	
	params = method_init(env, p, VCD_CAP_INIT, 1);
	method_return_if_fault(env);
	
	_params_orig = p;
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:i,s:i,*}",
		"name", &name,
		"reboot", &reboot,
		"wait", &wait);
	method_return_if_fault(env);
	
	if (!validate_name(name))
		method_return_fault(env, MEINVAL);
	
	if ((xid = vxdb_getxid(name)))
		method_return_fault(env, MEEXIST);
	
	if (vx_get_info(xid, NULL) == -1 && errno == ESRCH)
		method_return_fault(env, MESTOPPED);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT method,stop,timeout FROM init_method WHERE xid = %d",
		xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	if (dbi_result_get_numrows(dbr) < 1) {
		method  = "init";
		stop    = "";
		timeout = 30;
	}
	
	else {
		dbi_result_first_row(dbr);
		method  = dbi_result_get_string(dbr, "method");
		stop    = dbi_result_get_string(dbr, "stop");
		timeout = dbi_result_get_int(dbr, "timeout");
	}
	
	if (strcmp(method, "init") == 0)
		shutdown_init(env);
	
	else if (strcmp(method, "initng") == 0)
		shutdown_initng(env);
	
	else if (strcmp(method, "sysvrc") == 0)
		shutdown_sysvrc(env);
	
	else if (strcmp(method, "gentoo") == 0)
		shutdown_gentoo(env, stop);
	
	else
		method_return_faultf(env, MECONF, "unknown init style: %s", method);
	
	if (wait)
		signal(SIGCHLD, SIG_DFL);
	else
		signal(SIGCHLD, SIG_IGN);
	
	switch((pid = fork())) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
		break;
	
	case 0:
		wait_death(timeout);
		
		/* never get here */
		exit(EXIT_FAILURE);
	
	default:
		if (waitpid(pid, &status, wait == 0 ? WNOHANG : 0) == -1)
			method_return_faultf(env, MESYS, "%s: waitpid: %s", __FUNCTION__, strerror(errno));
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			method_return_faultf(env, MESYS, "init_init: %s", strerror(WEXITSTATUS(status)));
		
		if (WIFSIGNALED(status))
			kill(getpid(), WTERMSIG(status));
	}
	
	return params;
}
