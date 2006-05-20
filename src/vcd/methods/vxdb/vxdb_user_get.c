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

/* vxdb.user.get([string name]) */
XMLRPC_VALUE m_vxdb_user_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	XMLRPC_VALUE response;
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name = XMLRPC_VectorGetStringWithID(params, "name");
	
	if (name) {
		response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
		
		dbr = dbi_conn_queryf(vxdb,
			"SELECT uid,name,admin FROM user WHERE name = '%s'",
			name);
		
		if (!dbr)
			goto out;
		
		dbi_result_first_row(dbr);
		
		int uid = dbi_result_get_int(dbr, "uid");
		int admin = dbi_result_get_int(dbr, "admin") == 0 ? 0 : 1;
		
		XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("uid", uid));
		XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("name", name, 0));
		XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("admin", admin));
	}
	
	else {
		response = XMLRPC_CreateVector(NULL, xmlrpc_vector_array);
		
		dbr = dbi_conn_queryf(vxdb, "SELECT name FROM user ORDER BY name ASC");
		
		if (!dbr)
			goto out;
		
		while (dbi_result_next_row(dbr)) {
			char *name = (char *) dbi_result_get_string(dbr, "name");
			XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString(NULL, name, 0));
		}
	}
	
out:
	return response;
}
