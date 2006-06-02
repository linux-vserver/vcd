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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xmlrpc.h"

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vxdb.user.set(string name, string password[, int admin]) */
XMLRPC_VALUE m_vxdb_user_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *username = XMLRPC_VectorGetStringWithID(params, "username");
	char *password = XMLRPC_VectorGetStringWithID(params, "password");
	int admin;
	
	if (XMLRPC_VectorGetValueWithID(params, "admin") == NULL)
		admin = -1;
	else
		admin = XMLRPC_VectorGetIntWithID(params, "admin");
	
	if (!validate_username(username) || !validate_password(password))
		return method_error(MEREQ);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT uid,admin FROM user WHERE name = '%s'",
		username);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	int uid, adm;
	
	if (dbi_result_get_numrows(dbr) > 0) {
		dbi_result_first_row(dbr);
		uid = dbi_result_get_int(dbr, "uid");
		adm = admin == -1 ? dbi_result_get_int(dbr, "admin") : admin;
	}
	
	else {
		dbr = dbi_conn_queryf(vxdb, "SELECT uid FROM user ORDER BY uid DESC LIMIT 1");
		
		if (!dbr)
			return method_error(MEVXDB);
		
		if (dbi_result_get_numrows(dbr) > 0) {
			dbi_result_first_row(dbr);
			uid = dbi_result_get_int(dbr, "uid") + 1;
		}
		
		else
			return method_error(MEVXDB);
		
		adm = admin == -1 ? 0 : 1;
	}
	
	/* TODO: password to SHA-1 */
	
	dbr = dbi_conn_queryf(vxdb,
		"INSERT OR REPLACE INTO user (uid, name, password, admin) "
		"VALUES (%d, '%s', '%s', %d)",
		uid, username, password, adm);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	return NULL;
}
