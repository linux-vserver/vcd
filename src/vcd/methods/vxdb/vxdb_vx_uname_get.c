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

#include "lucid.h"

#include "auth.h"
#include "lists.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

xmlrpc_value *m_vxdb_vx_uname_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params, *response;
	char *name, *uname;
	xid_t xid;
	dbi_result dbr;
	int i;
	
	params = method_init(env, p, VCD_CAP_UNAME, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,*}",
		"name", &name,
		"uname", &uname);
	method_return_if_fault(env);
	
	method_empty_params(2, &name, &uname);
	
	response = xmlrpc_array_new(env);
	
	if (!name) {
		for (i = 0; vhiname_list[i].key; i++)
			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:s}",
				"uname", vhiname_list[i].key,
				"value", ""));
		
		return response;
	}
	
	else {
		if (!validate_name(name) || (uname && !validate_uname(uname)))
			method_return_fault(env, MEINVAL);
		
		if (!(xid = vxdb_getxid(name)))
			method_return_fault(env, MENOVPS);
		
		if (uname)
			dbr = dbi_conn_queryf(vxdb,
				"SELECT uname,value FROM vx_uname WHERE xid = %d AND uname = '%s'",
				xid, uname);
		
		else
			dbr = dbi_conn_queryf(vxdb,
				"SELECT uname,value FROM vx_uname WHERE xid = %d",
				xid);
		
		if (!dbr)
			method_return_fault(env, MEVXDB);
		
		while (dbi_result_next_row(dbr))
			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:s}",
				"uname", dbi_result_get_string(dbr, "uname"),
				"value", dbi_result_get_string(dbr, "value")));
	}
	
	method_return_if_fault(env);
	return response;
}
