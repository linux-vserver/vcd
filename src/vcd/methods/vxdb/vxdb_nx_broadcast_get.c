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

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

xmlrpc_value *m_vxdb_nx_broadcast_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params, *response = NULL;
	char *name;
	xid_t xid;
	vxdb_result *dbr;
	int rc;
	
	params = method_init(env, p, c, VCD_CAP_NET, M_OWNER);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,*}",
		"name", &name);
	method_return_if_fault(env);
	
	if (!validate_name(name))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	rc = vxdb_prepare(&dbr,
		"SELECT broadcast FROM nx_broadcast "
		"WHERE xid = %d",
		xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else {
		rc = vxdb_step(dbr);
		
		if (rc == -1)
			method_set_fault(env, MEVXDB);
		
		else if (rc == 0)
			response = xmlrpc_build_value(env, "s", "");
		
		else
			response = xmlrpc_build_value(env, "s", sqlite3_column_text(dbr, 0));
	}
	
	sqlite3_finalize(dbr);
	
	return response;
}
