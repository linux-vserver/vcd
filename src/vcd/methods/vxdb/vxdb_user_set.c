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

#include <string.h>

#include "xmlrpc.h"

#include "auth.h"
#include "lists.h"
#include "methods.h"
#include "vxdb.h"

/* vxdb.user.set(string name[, string password[, int admin]]) */
XMLRPC_VALUE m_vxdb_user_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name     = XMLRPC_VectorGetStringWithID(params, "name");
	char *password = XMLRPC_VectorGetStringWithID(params, "password");
	int admin;
	
	if (XMLRPC_VectorGetValueWithID(params, "admin") == NULL)
		admin = -1;
	else
		admin = XMLRPC_VectorGetIntWithID(params, "admin");
	
	if (!name)
		return method_error(MEREQ);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT uid FROM user WHERE name = '%s'",
		name);
	
	if (dbr && dbi_result_get_numrows(dbr) > 0) {
		int uid = dbi_result_get_int(dbr, "uid");
		
		if (password)
			dbi_conn_queryf(vxdb,
				"UPDATE user SET password = '%s' WHERE uid = %d",
				password, uid);
		
		if (admin != -1)
			dbi_conn_queryf(vxdb,
				"UPDATE user SET admin = %d WHERE uid = %d",
				admin == 0 ? 0 : 1, uid);
	}
	
	else
		return method_error(MENOENT);
	
	return params;
}
