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

#include "xmlrpc.h"

#include "auth.h"
#include "methods.h"
#include "vxdb.h"

/* vxdb.list() */
XMLRPC_VALUE m_vxdb_list(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	dbi_result dbr;
	XMLRPC_VALUE response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	if (auth_isadmin(r)) {
		dbr = dbi_conn_queryf(vxdb,
			"SELECT name FROM xid_name_map ORDER BY name ASC");
	}
	
	else {
		int uid = auth_getuid(r);
		
		dbr = dbi_conn_queryf(vxdb,
			"SELECT xid_name_map.name AS name FROM xid_name_map "
			"INNER JOIN xid_uid_map "
			"ON xid_name_map.xid = xid_uid_map.xid "
			"WHERE xid_uid_map.uid = %d "
			"ORDER BY name ASC",
			uid);
	}
	
	if (!dbr)
		return method_error(MEVXDB);
	
	while (dbi_result_next_row(dbr)) {
		XMLRPC_AddValueToVector(response,
			XMLRPC_CreateValueString(NULL, dbi_result_get_string(dbr, "name"), 0));
	}
	
	return response;
}
