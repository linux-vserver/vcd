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

/* vxdb.vx.ccaps.get([string name]) */
xmlrpc_value *m_vxdb_vx_ccaps_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params, *response;
	char *name;
	xid_t xid;
	dbi_result dbr;
	int i;
	
	params = method_init(env, p, VCD_CAP_CCAP, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,*}",
		"name", &name);
	method_return_if_fault(env);
	
	method_empty_params(1, &name);
	
	response = xmlrpc_array_new(env);
	
	if (name) {
		if (!validate_name(name))
			method_return_fault(env, MEINVAL);
	
		if (!(xid = vxdb_getxid(name)))
			method_return_fault(env, MENOVPS);
		
		dbr = dbi_conn_queryf(vxdb,
			"SELECT ccap FROM vx_ccaps WHERE xid = %d",
			xid);
		
		if (!dbr)
			method_return_fault(env, MEVXDB);
		
		while (dbi_result_next_row(dbr))
			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"s", dbi_result_get_string(dbr, "ccap")));
	}
	
	else {
		for (i = 0; ccaps_list[i].key; i++)
			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"s", ccaps_list[i].key));
	}
	
	method_return_if_fault(env);
	return response;
}
