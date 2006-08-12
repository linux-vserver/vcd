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
#include "lists.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

xmlrpc_value *m_vxdb_vx_limit_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params, *response;
	char *name, *limit;
	xid_t xid;
	vxdb_result *dbr;
	int i, rc;
	
	params = method_init(env, p, c, VCD_CAP_RLIM, M_OWNER);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,*}",
		"name", &name,
		"limit", &limit);
	method_return_if_fault(env);
	
	method_empty_params(2, &name, &limit);
	
	response = xmlrpc_array_new(env);
	
	if (!name) {
		for (i = 0; rlimit_list[i].key; i++)
			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:i,s:i}",
				"limit", rlimit_list[i].key,
				"soft",  0,
				"max",   0));
	}
	
	else {
		if (!validate_name(name) || (limit && !validate_rlimit(limit)))
			method_return_fault(env, MEINVAL);
		
		if (!(xid = vxdb_getxid(name)))
			method_return_fault(env, MENOVPS);
		
		if (limit)
			rc = vxdb_prepare(&dbr,
				"SELECT type,soft,max FROM vx_limit "
				"WHERE xid = %d AND type = '%s'",
				xid, limit);
		
		else
			rc = vxdb_prepare(&dbr,
				"SELECT type,soft,max FROM vx_limit "
				"WHERE xid = %d",
				xid);
		
		if (rc)
			method_set_fault(env, MEVXDB);
		
		else {
			vxdb_foreach_step(rc, dbr)
				xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
					"{s:s,s:i,s:i}",
					"limit", sqlite3_column_text(dbr, 0),
					"soft",  sqlite3_column_int(dbr, 1),
					"max",   sqlite3_column_int(dbr, 2)));
			
			if (rc == -1)
				method_set_fault(env, MEVXDB);
		}
		
		sqlite3_finalize(dbr);
	}
	
	return response;
}
