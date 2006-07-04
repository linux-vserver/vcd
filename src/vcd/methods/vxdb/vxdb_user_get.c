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
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

xmlrpc_value *m_vxdb_user_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params, *response;
	char *user;
	dbi_result dbr;
	
	params = method_init(env, p, VCD_CAP_AUTH, 0);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,*}",
		"username", &user);
	method_return_if_fault(env);
	
	method_empty_params(1, &user);
	
	if (user) {
		if (!validate_username(user))
			method_return_fault(env, MEINVAL);
		
		dbr = dbi_conn_queryf(vxdb,
			"SELECT uid,name,admin FROM user WHERE name = '%s'",
			user);
	}
	
	else
		dbr = dbi_conn_queryf(vxdb,
			"SELECT uid,name,admin FROM user ORDER BY name ASC");
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	response = xmlrpc_array_new(env);
	
	while (dbi_result_next_row(dbr))
		xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
			"{s:s,s:i,s:i}",
			"username", dbi_result_get_string(dbr, "name"),
			"uid",      dbi_result_get_int(dbr, "uid"),
			"admin",    dbi_result_get_int(dbr, "admin")));
	
	method_return_if_fault(env);
	
	return response;
}
