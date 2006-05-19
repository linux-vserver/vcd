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

/* vxdb.owner.remove(string name[, string username]) */
XMLRPC_VALUE m_vxdb_owner_remove(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = get_method_params(r);
	
	if (!auth_isadmin(r))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	char *name = XMLRPC_VectorGetStringWithID(params, "name");
	char *user = XMLRPC_VectorGetStringWithID(params, "username");
	
	if (!name)
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	if (vxdb_getxid(name, &xid) == -1)
		return XMLRPC_UtilityCreateFault(404, "Not Found");
	
	if (user) {
		dbr = dbi_conn_queryf(vxdb,
			"SELECT uid FROM user WHERE name = '%s'",
			user);
		
		if (dbi_result_get_numrows(dbr) == 0)
			return XMLRPC_UtilityCreateFault(404, "Not Found");
		
		dbi_result_first_row(dbr);
		int uid = dbi_result_get_int(dbr, "uid");
		
		dbr = dbi_conn_queryf(vxdb,
			"SELECT uid FROM xid_uid_map WHERE uid = %d AND xid = %d",
			uid, xid);
		
		if (dbi_result_get_numrows(dbr) == 0)
			return XMLRPC_UtilityCreateFault(404, "Not Found");
		
		dbr = dbi_conn_queryf(vxdb,
			"DELETE FROM xid_uid_map WHERE xid = %d AND uid = %d",
			xid, uid);
	}
	
	else
		dbr = dbi_conn_queryf(vxdb,
			"DELETE FROM xid_uid_map WHERE xid = %d",
			xid);
	
	if (!dbr)
		return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	
	return params;
}