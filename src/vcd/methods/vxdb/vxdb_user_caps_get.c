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

xmlrpc_value *m_vxdb_user_caps_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params, *response;
	char *user;
	int uid;
	dbi_result dbr;
	int i;
	
	params = method_init(env, p, VCD_CAP_AUTH, 0);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,*}",
		"username", &user);
	method_return_if_fault(env);
	
	method_empty_params(1, &user);
	
	response = xmlrpc_array_new(env);
	
	if (user) {
		if (!validate_username(user))
			method_return_fault(env, MEINVAL);
	
		if (!(uid = auth_getuid(user)))
			method_return_fault(env, MENOUSER);
		
		dbr = dbi_conn_queryf(vxdb,
			"SELECT cap FROM user_caps WHERE uid = %d",
			uid);
		
		if (!dbr)
			method_return_fault(env, MEVXDB);
		
		while (dbi_result_next_row(dbr))
			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"s", dbi_result_get_string(dbr, "cap")));
		
		method_return_if_fault(env);
	}
	
	else {
		for (i = 0; vcd_caps_list[i].key; i++)
			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"s", vcd_caps_list[i].key));
	}
	
	return response;
}
