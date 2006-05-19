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

/* vxdb.owner.get(string name) */
XMLRPC_VALUE m_vxdb_owner_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = get_method_params(r);
	XMLRPC_VALUE response;
	
	if (!auth_isadmin(r))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	char *name = XMLRPC_VectorGetStringWithID(params, "name");
	response   = XMLRPC_CreateVector(NULL, xmlrpc_vector_array);
	
	if (!name)
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	if (vxdb_getxid(name, &xid) == -1)
		return XMLRPC_UtilityCreateFault(404, "Not Found");
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT user.name AS user FROM user "
		"INNER JOIN xid_uid_map "
		"ON user.uid = xid_uid_map.uid "
		"WHERE xid_uid_map.xid = %d",
		xid);
	
	if (!dbr)
		goto out;
	
	while (dbi_result_next_row(dbr)) {
		char *user = (char *) dbi_result_get_string(dbr, "user");
		XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString(NULL, user, 0));
	}
	
out:
	return response;
}
