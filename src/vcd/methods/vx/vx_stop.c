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
#include <vserver.h>
#include <sys/wait.h>

#include "lucid.h"

#include "auth.h"
#include "cfg.h"
#include "log.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* stop/restart process:
   1) run init-style based stop command (background)
   1) create new vps killer
   3) wait for vps killer if approriate
*/

static xid_t xid = 0;
static const char *name = NULL;

static
xmlrpc_value *shutdown_init(xmlrpc_env *env)
{
	pid_t pid;
	int i, status;
	
	char *vserverdir = cfg_getstr(cfg, "vserverdir");
	char *vdir = NULL;
	
	asprintf(&vdir, "%s/%s", vserverdir, name);
	
	switch ((pid = fork())) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
	
	case 0:
		usleep(200);
		
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
		
		if (exec_replace("/sbin/telinit 0") == -1)
			exit(errno);
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, WNOHANG) == -1)
			method_return_faultf(env, MESYS, "%s: waitpid: %s", __FUNCTION__, strerror(errno));
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			method_return_faultf(env, MESYS, "shutdown failed: %s", strerror(WEXITSTATUS(status)));
	}
	
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
	return NULL;
}

/* vx.stop(string name[, int wait[, int reboot]) */
xmlrpc_value *m_vx_stop(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	const char *method, *stop;
	int wait, reboot, timeout;
	dbi_result dbr;
	
	params = method_init(env, p, VCD_CAP_INIT, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:i,s:i,*}",
		"name", &name,
		"reboot", &reboot,
		"wait", &wait);
	method_return_if_fault(env);
	
	if (!validate_name(name))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	if (vx_get_info(xid, NULL) == -1) {
		if (errno == ESRCH)
			method_return_fault(env, MESTOPPED);
		else
			method_return_faultf(env, MESYS, "vx_get_info: %s", strerror(errno));
	}
	
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
	
	m_vx_killer(env, p, c);
	method_return_if_fault(env);
	
	return xmlrpc_nil_new(env);
}
