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

#include "lucid.h"
#include "xmlrpc.h"

#include "auth.h"
#include "cfg.h"
#include "log.h"
#include "methods.h"
#include "vxdb.h"

/* stop/restart process:
   1) run init-style based stop command (background)
   1) create new vps killer
   3) wait for vps killer if approriate
*/

static xid_t xid = 0;
static char *name = NULL;

/* vx.stop(string name[, int wait[, int reboot]) */
XMLRPC_VALUE m_vx_stop(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	dbi_result dbr;
	char *method, *stop;
	XMLRPC_VALUE params = method_get_params(r);
	XMLRPC_VALUE response;
	
	if (!auth_isadmin(r) && !auth_isowner(r))
		return method_error(MEPERM);
	
	name = XMLRPC_VectorGetStringWithID(params, "name");
	
	if (!name)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	if (vx_get_info(xid, NULL) == -1)
		return method_error(MESTOPPED);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT method,stop FROM init_method WHERE xid = %d",
		xid);
	
	if (dbi_result_get_numrows(dbr) < 1) {
		method  = "init";
		stop    = "";
	}
	
	else {
		dbi_result_first_row(dbr);
		method  = (char *) dbi_result_get_string(dbr, "method");
		stop    = (char *) dbi_result_get_string(dbr, "stop");
	}
	
	pid_t pid;
	int i, status;
	
	char *vdirbase = cfg_getstr(cfg, "vserver-basedir");
	char *vdir = NULL;
	
	asprintf(&vdir, "%s/%s", vdirbase, name);
	
	signal(SIGCHLD, SIG_DFL);
	
	switch ((pid = fork())) {
	case -1:
		return method_error(MESYS);
	
	case 0:
		usleep(200);
		
		for (i = 0; i < 100; i++)
			close(i);
		
		if (vx_enter_namespace(xid) == -1 ||
		    chroot(vdir) == -1 ||
		    nx_migrate(xid) == -1 ||
		    vx_migrate(xid, NULL) == -1)
			exit(errno);
		
		if (strcmp(method, "init") == 0) {
			if (!stop || !*stop)
				stop = "0";
			
			if (exec_replace("/sbin/telinit %s", stop) == -1)
				exit(errno);
		}
		
		else if (strcmp(method, "gentoo") == 0) {
			if (!stop || !*stop)
				stop = "shutdown";
			
			if (exec_fork("/sbin/rc %s", stop) == -1)
				exit(errno);
		}
		
		else
			exit(MECONF);
		
		exit(MESYS);
	
	default:
		switch (waitpid(pid, &status, WNOHANG)) {
		case -1:
			return method_error(MESYS);
		
		case 0:
			signal(SIGCHLD, SIG_IGN);
			break;
		
		default:
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				return method_error(MESYS);
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
		}
	}
	
	response = m_vx_killer(s, r, d);
	
	char *fault_string = XMLRPC_VectorGetStringWithID(response, "faultString");
	int   fault_code   = XMLRPC_VectorGetIntWithID(response, "faultCode");
	
	if (fault_string || fault_code != 0)
		return response;
	
	return params;
}