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

/* vxdb.vx.limit.set(string name, string type, int min, int soft, int max) */
XMLRPC_VALUE m_vxdb_vx_limit_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	char *name = XMLRPC_VectorGetStringWithID(params, "name");
	char *type = XMLRPC_VectorGetStringWithID(params, "type");
	
	int min  = XMLRPC_VectorGetIntWithID(params, "min");
	int soft = XMLRPC_VectorGetIntWithID(params, "soft");
	int max  = XMLRPC_VectorGetIntWithID(params, "max");
	
	if (!name || !type || flist32_getval(rlimit_list, type, NULL) == -1)
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	if (vxdb_getxid(name, &xid) == -1)
		return XMLRPC_UtilityCreateFault(404, "Not Found");
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT min FROM vx_limit WHERE xid = %d AND type = '%s'",
		xid, type);
	
	if (dbr) {
		if (dbi_result_get_numrows(dbr) > 0)
			dbr = dbi_conn_queryf(vxdb,
				"UPDATE vx_limit SET min = %d, soft = %d, max = %d "
				"WHERE xid = %d AND type = '%s'",
				min, soft, max, xid, type);
		else
			dbr = dbi_conn_queryf(vxdb,
				"INSERT INTO vx_limit (xid, type, min, soft, max) "
				"VALUES (%d, '%s', %d, %d, %d)",
				xid, type, min, soft, max);
	}
	
	if (!dbr)
		return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	
	return params;
}
