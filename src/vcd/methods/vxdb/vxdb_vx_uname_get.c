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
	xmlrpc_value *params, *response = NULL;
	char *name, *uname;
	xid_t xid;
	vxdb_result *dbr;
	int i, rc;
	
	params = method_init(env, p, VCD_CAP_UNAME, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,*}",
		"name", &name,
		"uname", &uname);
	method_return_if_fault(env);
	
	method_empty_params(2, &name, &uname);
	
	if (!name) {
		response = xmlrpc_array_new(env);
		
		for (i = 0; vhiname_list[i].key; i++)
			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:s}",
				"uname", vhiname_list[i].key,
				"value", ""));
	}
	
	else {
		if (!validate_name(name) || (uname && !validate_uname(uname)))
			method_return_fault(env, MEINVAL);
		
		if (!(xid = vxdb_getxid(name)))
			method_return_fault(env, MENOVPS);
		
		if (uname)
			rc = vxdb_prepare(&dbr,
				"SELECT uname,value FROM vx_uname WHERE xid = %d AND uname = '%s'",
				xid, uname);
		
		else
			rc = vxdb_prepare(&dbr,
				"SELECT uname,value FROM vx_uname WHERE xid = %d",
				xid);
		
		if (rc)
			method_set_fault(env, MEVXDB);
		
		else {
			response = xmlrpc_array_new(env);
			
			for (rc = vxdb_step(dbr); rc == 1; rc = vxdb_step(dbr))
				xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
					"{s:s,s:s}",
					"uname", sqlite3_column_text(dbr, 0),
					"value", sqlite3_column_text(dbr, 0)));
			
			if (rc == -1)
				method_set_fault(env, MEVXDB);
		}
		
		sqlite3_finalize(dbr);
	}
	
	return response;
}
