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

#include <lucid/str.h>

#include "auth.h"
#include "log.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

xmlrpc_value *m_vxdb_init_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	TRACEIT
	
	xmlrpc_value *params;
	char *name, *init, *halt, *reboot;
	int rc;
	xid_t xid;
	
	params = method_init(env, p, c, VCD_CAP_INIT, M_OWNER|M_LOCK);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,s:s,s:s,*}",
		"name", &name,
		"init", &init,
		"halt", &halt,
		"reboot", &reboot);
	method_return_if_fault(env);
	
	method_empty_params(3, &init, &halt, &reboot);
	
	if (!init)
		init = "/sbin/init";
	
	if (!halt)
		halt = "/sbin/halt";
	
	if (!reboot)
		reboot = "/sbin/reboot";
	
	if (!validate_name(name) ||
	    !str_path_isabs(init) ||
	    !str_path_isabs(halt) ||
	    !str_path_isabs(reboot))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	rc = vxdb_exec(
		"INSERT OR REPLACE INTO init (xid, init, halt, reboot) "
		"VALUES (%d, '%s', '%s', '%s')",
		xid, init, halt, reboot);
	
	if (rc)
		method_return_fault(env, MEVXDB);
	
	return xmlrpc_nil_new(env);
}
