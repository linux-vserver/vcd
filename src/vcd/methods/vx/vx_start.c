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
#include <string.h>
#include <errno.h>
#include <vserver.h>
#include <sys/wait.h>

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

#define _LUCID_PRINTF_MACROS
#include <lucid/log.h>
#include <lucid/mem.h>
#include <lucid/printf.h>
#include <lucid/str.h>

/* start process:
   1) create network context (helper will do the rest)
   2) create context (helper will do the rest)
*/

/* vx.start(string name) */
xmlrpc_value *m_vx_start(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME
	
	xmlrpc_value *params;
	char *name;
	xid_t xid;
	pid_t pid;
	int status;
	
	nx_flags_t ncf = {
		.flags = NXF_PERSISTENT|NXF_STATE_ADMIN|NXF_SC_HELPER,
	};
	
	vx_flags_t vcf = {
		.flags = VXF_PERSISTENT|VXF_STATE_ADMIN|VXF_SC_HELPER|
		         VXF_INFO_INIT|VXF_REBOOT_KILL,
	};
	
	params = method_init(env, p, c, VCD_CAP_INIT, M_OWNER|M_LOCK);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,*}",
		"name", &name);
	method_return_if_fault(env);
	
	if (!validate_name(name))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	if (vx_info(xid, NULL) != -1)
		method_return_fault(env, MERUNNING);
	
	int pfds[2];
	char *buf = "(null)";
	
	if (pipe(pfds) == -1)
		method_return_sys_fault(env, "pipe");
	
	switch ((pid = fork())) {
	case -1:
		close(pfds[0]);
		close(pfds[1]);
		method_return_faultf(env, MESYS, "fork: %s", strerror(errno));
	
	case 0:
		close(pfds[0]);
		
		dup2(pfds[1], STDOUT_FILENO);
		dup2(pfds[1], STDERR_FILENO);
		
		if (nx_create(xid, &ncf) == -1) {
			printf("nx_create: %s", strerror(errno));
			log_perror_and_die("nx_create");
		}
		
		if (vx_create(xid, &vcf) == -1) {
			printf("vx_create: %s", strerror(errno));
			log_perror_and_die("vx_create");
		}
		
		exit(EXIT_SUCCESS);
	
	default:
		close(pfds[1]);
		
		if (str_readfile(pfds[0], &buf) == -1)
			method_set_sys_fault(env, "str_readfile");
		
		close(pfds[0]);
		
		if (waitpid(pid, &status, 0) == -1)
			method_set_sys_fault(env, "waitpid");
		
		else if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			method_set_faultf(env, MESYS,
					"startup failed: %s", buf);
		
		mem_free(buf);
	}
	
	return xmlrpc_nil_new(env);
}
