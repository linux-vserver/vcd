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
#include "validate.h"
#include "vxdb.h"

/* vxdb.init.mount.get(string name[, string path]) */
XMLRPC_VALUE m_vxdb_mount_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	XMLRPC_VALUE response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	const char *name = XMLRPC_VectorGetStringWithID(params, "name");
	const char *path = XMLRPC_VectorGetStringWithID(params, "path");
	
	if (!validate_name(name) || (path && !validate_path(path)))
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	if (path) {
		dbr = dbi_conn_queryf(vxdb,
			"SELECT spec,mntops,vfstype FROM mount "
			"WHERE xid = %d AND file = '%s'",
			xid, path);
		
		if (!dbr)
			return method_error(MEVXDB);
		
		if (dbi_result_get_numrows(dbr) < 1)
			return method_error(MENOENT);
		
		dbi_result_first_row(dbr);
		
		XMLRPC_AddValueToVector(response,
			XMLRPC_CreateValueString("spec", dbi_result_get_string(dbr, "spec"), 0));
		XMLRPC_AddValueToVector(response,
			XMLRPC_CreateValueString("opts", dbi_result_get_string(dbr, "mntops"), 0));
		XMLRPC_AddValueToVector(response,
			XMLRPC_CreateValueString("type", dbi_result_get_string(dbr, "vfstype"), 0));
	}
	
	else {
		dbr = dbi_conn_queryf(vxdb,
			"SELECT file FROM mount WHERE xid = %d ORDER BY file ASC",
			xid);
		
		if (!dbr)
			return method_error(MEVXDB);
		
		while (dbi_result_next_row(dbr)) {
			XMLRPC_AddValueToVector(response,
				XMLRPC_CreateValueString(NULL, dbi_result_get_string(dbr, "file"), 0));
		}
	}
	
	return response;
}
