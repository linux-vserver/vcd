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
#include <sys/wait.h>

#include "auth.h"
#include <lucid/log.h>
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* helper.shutdown(string name) */
xmlrpc_value *m_helper_shutdown(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME
	
	xmlrpc_value *params;
	xid_t xid;
	int rc, status;
	pid_t pid;
	char *name;
	
	params = method_init(env, p, c, VCD_CAP_HELPER, 0);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:i,*}",
		"xid", &xid);
	method_return_if_fault(env);
	
	if (!(name = vxdb_getname(xid)))
		method_return_fault(env, MENOVPS);
	
	rc = vxdb_exec("DELETE FROM restart WHERE xid = %d", xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	if (sqlite3_changes(vxdb) < 1)
		return xmlrpc_nil_new(env);
	
	params = xmlrpc_build_value(env, "{s:s}", "name", name);
	
	switch((pid = fork())) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
		break;
	
	case 0:
		vx_wait(xid, NULL);
		m_vx_start(env, params, METHOD_INTERNAL);
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, WNOHANG) == -1)
			method_return_faultf(env, MESYS, "%s: waitpid: %s", __FUNCTION__, strerror(errno));
	}
	
	return xmlrpc_nil_new(env);
}
