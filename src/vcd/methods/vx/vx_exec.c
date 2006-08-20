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
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <vserver.h>
#include <sys/wait.h>
#include <lucid/chroot.h>
#include <lucid/exec.h>
#include <lucid/io.h>

#include "auth.h"
#include "cfg.h"
#include "log.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vx.exec(string name, string command) */
xmlrpc_value *m_vx_exec(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	TRACEIT
	
	xmlrpc_value *params;
	char *name, *command, *output, *vserverdir, vdir[PATH_MAX];
	xid_t xid;
	int outfds[2], status;
	pid_t pid;
	
	params = method_init(env, p, c, VCD_CAP_EXEC, M_OWNER);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,*}",
		"name", &name,
		"command", &command);
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
	
	vserverdir = cfg_getstr(cfg, "vserverdir");
	snprintf(vdir, PATH_MAX, "%s/%s", vserverdir, name);
	
	if (vx_enter_namespace(xid) == -1)
		method_return_faultf(env, MESYS, "vx_enter_namespace: %s", strerror(errno));
	
	else if (chroot_secure_chdir(vdir, "/") == -1)
		method_return_faultf(env, MESYS, "chroot_secure_chdir: %s", strerror(errno));
	
	else if (chroot(".") == -1)
		method_return_faultf(env, MESYS, "chroot: %s", strerror(errno));
	
	else if (nx_migrate(xid) == -1)
		method_return_faultf(env, MESYS, "nx_migrate: %s", strerror(errno));
	
	else {
		if (pipe(outfds) == -1)
			method_return_faultf(env, MESYS, "pipe: %s", strerror(errno));
		
		switch ((pid = fork())) {
		case -1:
			log_error("fork: %s", strerror(errno));
			break;
		
		case 0:
			usleep(100);
			
			close(outfds[0]);
			
			dup2(outfds[1], STDOUT_FILENO);
			dup2(outfds[1], STDERR_FILENO);
			
			clearenv();
			
			if (vx_migrate(xid, NULL) == -1)
				log_error("vx_migrate: %s", strerror(errno));
			
			else if (exec_replace(command) == -1)
				log_error("exec_replace: %s", strerror(errno));
			
			exit(EXIT_FAILURE);
		
		default:
			close(outfds[1]);
			
			if (io_read_eof(outfds[0], &output) == -1)
				method_return_faultf(env, MESYS, "io_read_eof: %s", strerror(errno));
			
			close(outfds[0]);
			
			if (waitpid(pid, &status, 0) == -1)
				method_return_faultf(env, MESYS, "waitpid: %s", strerror(errno));
			
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				method_return_faultf(env, MESYS, "command failed:\n%s", output);
		}
	}
	
	return xmlrpc_build_value(env, "s", output);
}
