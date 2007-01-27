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
#include <sys/wait.h>

#include "auth.h"
#include "methods.h"
#include "vxdb.h"

#include <lucid/chroot.h>
#include <lucid/exec.h>
#include <lucid/log.h>
#include <lucid/str.h>

/* vx.exec(string name, string command) */
xmlrpc_value *m_vx_exec(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name, *command, *output;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_EXEC, M_OWNER);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,*}",
			"name", &name,
			"command", &command);
	method_return_if_fault(env);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	if (vx_info(xid, NULL) == -1) {
		if (errno == ESRCH)
			method_return_fault(env, MESTOPPED);
		else
			method_return_sys_fault(env, "vx_info");
	}

	const char *vdir = vxdb_getvdir(name);

	if (ns_enter(xid, 0) == -1)
		method_return_sys_fault(env, "vx_enter_namespace");

	else if (chroot_secure_chdir(vdir, "/") == -1)
		method_return_sys_fault(env, "chroot_secure_chdir");

	else if (chroot(".") == -1)
		method_return_sys_fault(env, "chroot");

	else if (nx_migrate(xid) == -1)
		method_return_sys_fault(env, "nx_migrate");

	else {
		int pfds[2];
		
		if (pipe(pfds) == -1)
			method_return_sys_fault(env, "pipe");

		pid_t pid;
		int status;

		switch ((pid = fork())) {
		case -1:
			close(pfds[0]);
			close(pfds[1]);
			method_return_sys_fault(env, "fork");
			break;

		case 0:
			close(pfds[0]);

			dup2(pfds[1], STDOUT_FILENO);
			dup2(pfds[1], STDERR_FILENO);

			clearenv();

			if (vx_migrate(xid, NULL) == -1)
				log_perror("vx_migrate");

			else if (exec_replace(command) == -1)
				log_perror("exec_replace");

			exit(EXIT_FAILURE);

		default:
			close(pfds[1]);

			if (str_readfile(pfds[0], &output) == -1)
				method_return_sys_fault(env, "io_read_eof");

			close(pfds[0]);

			if (waitpid(pid, &status, 0) == -1)
				method_return_sys_fault(env, "waitpid");

			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				method_return_faultf(env, MESYS,
						"command failed with exit code %d:\n%s",
						WEXITSTATUS(status), output);
		}
	}

	return xmlrpc_build_value(env, "s", output);
}
