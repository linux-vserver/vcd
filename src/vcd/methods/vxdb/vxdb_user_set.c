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

/* vxdb.user.set(string name, string password, int admin) */
xmlrpc_value *m_vxdb_user_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	char *user, *pass;
	int uid, admin;
	dbi_result dbr;
	
	params = method_init(env, p, VCD_CAP_AUTH, 0);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,s:i,*}",
		"username", &user,
		"password", &pass,
		"admin", &admin);
	method_return_if_fault(env);
	
	method_empty_params(1, &pass);
	
	if (!validate_username(user))
		method_return_fault(env, MEINVAL);
	
	uid = auth_getuid(user);
	
	if (uid == 0) {
		if (!validate_password(pass))
			method_return_fault(env, MEINVAL);
		
		dbr = dbi_conn_queryf(vxdb, "SELECT uid FROM user ORDER BY uid DESC LIMIT 1");
		
		if (!dbr)
			method_return_fault(env, MEVXDB);
		
		if (dbi_result_get_numrows(dbr) > 0) {
			dbi_result_first_row(dbr);
			uid = dbi_result_get_int(dbr, "uid");
		}
		
		uid++;
		
		dbr = dbi_conn_queryf(vxdb,
			"INSERT INTO user (uid, name, password, admin) "
			"VALUES (%d, '%s', '%s', %d)",
			uid, user, pass, admin);
		
		if (!dbr)
			method_return_fault(env, MEVXDB);
	}
	
	else {
		dbr = dbi_conn_queryf(vxdb,
			"UPDATE user SET admin = %d WHERE uid = %d",
			admin, uid);
		
		if (!dbr)
			method_return_fault(env, MEVXDB);
		
		if (pass) {
			dbr = dbi_conn_queryf(vxdb,
				"UPDATE user SET password = '%s' WHERE uid = %d",
				pass, uid);
			
			if (!dbr)
				method_return_fault(env, MEVXDB);
		}
	}
	
	return xmlrpc_nil_new(env);
}
