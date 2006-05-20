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

/* vxdb.dx.limit.set(string name, string type, int space, int inodes, int reserved) */
XMLRPC_VALUE m_vxdb_dx_limit_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name = XMLRPC_VectorGetStringWithID(params, "name");
	char *path = XMLRPC_VectorGetStringWithID(params, "path");
	
	int space    = XMLRPC_VectorGetIntWithID(params, "space");
	int inodes   = XMLRPC_VectorGetIntWithID(params, "inodes");
	int reserved = XMLRPC_VectorGetIntWithID(params, "reserved");
	
	if (!name || !path)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT space FROM dx_limit WHERE xid = %d AND path = '%s'",
		xid, path);
	
	if (dbr) {
		if (dbi_result_get_numrows(dbr) > 0)
			dbr = dbi_conn_queryf(vxdb,
				"UPDATE dx_limit SET space = %d, inodes = %d, reserved = %d "
				"WHERE xid = %d AND path = '%s'",
				space, inodes, reserved, xid, path);
		else
			dbr = dbi_conn_queryf(vxdb,
				"INSERT INTO dx_limit (xid, path, space, inodes, reserved) "
				"VALUES (%d, '%s', %d, %d, %d)",
				xid, path, space, inodes, reserved);
	}
	
	if (!dbr)
		return method_error(MEVXDB);
	
	return params;
}
