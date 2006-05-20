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

/* vxdb.vx.uname.set(string name, string field, string value) */
XMLRPC_VALUE m_vxdb_vx_uname_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name  = XMLRPC_VectorGetStringWithID(params, "name");
	char *field = XMLRPC_VectorGetStringWithID(params, "field");
	char *value = XMLRPC_VectorGetStringWithID(params, "value");
	
	if (!name || !field || !value || flist32_getval(vhiname_list, field, NULL) == -1)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT %s FROM vx_uname WHERE xid = %d",
		field, xid);
	
	if (dbr) {
		if (dbi_result_get_numrows(dbr) > 0)
			dbr = dbi_conn_queryf(vxdb,
				"UPDATE vx_uname SET %s = '%s' WHERE xid = %d",
				field, value, xid);
		else
			dbr = dbi_conn_queryf(vxdb,
				"INSERT INTO vx_uname (xid, %s) VALUES (%d, '%s')",
				field, xid, value);
	}
	
	if (!dbr)
		return method_error(MEVXDB);
	
	return params;
}
