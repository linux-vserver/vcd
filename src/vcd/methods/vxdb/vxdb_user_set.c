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

#include <stdlib.h>
#include <lucid/sha1.h>

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

xmlrpc_value *m_vxdb_user_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	char *user, *pass, *sha1_pass;
	int uid, admin, rc;
	vxdb_result *dbr;
	
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
		
		rc = vxdb_prepare(&dbr, "SELECT uid FROM user ORDER BY uid DESC LIMIT 1");
		
		if (rc)
			method_set_fault(env, MEVXDB);
		
		else {
			rc = vxdb_step(dbr);
			
			if (rc == -1)
				method_set_fault(env, MEVXDB);
			
			else if (rc == 1)
				uid = sqlite3_column_int(dbr, 0);
			
			uid++;
			
			sqlite3_finalize(dbr);
		
			sha1_pass = sha1_digest(pass);
			
			rc = vxdb_exec(
				"INSERT INTO user (uid, name, password, admin) "
				"VALUES (%d, '%s', '%s', %d)",
				uid, user, sha1_pass, admin);
			
			free(sha1_pass);
			
			if (rc)
				method_set_fault(env, MEVXDB);
		}
	}
	
	else {
		rc = vxdb_exec(
			"UPDATE user SET admin = %d WHERE uid = %d",
			admin, uid);
		
		if (rc)
			method_set_fault(env, MEVXDB);
		
		else if (pass) {
			sha1_pass = sha1_digest(pass);
			
			rc = vxdb_exec(
				"UPDATE user SET password = '%s' WHERE uid = %d",
				sha1_pass, uid);
			
			free(sha1_pass);
			
			if (rc)
				method_set_fault(env, MEVXDB);
		}
	}
	
	return xmlrpc_nil_new(env);
}
