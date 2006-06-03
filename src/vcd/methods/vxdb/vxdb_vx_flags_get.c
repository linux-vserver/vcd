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
#include "lists.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vxdb.vx.flags.get([string name]) */
XMLRPC_VALUE m_vxdb_vx_flags_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	XMLRPC_VALUE response = XMLRPC_CreateVector(NULL, xmlrpc_vector_array);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	const char *name = XMLRPC_VectorGetStringWithID(params, "name");
	const char *flag = XMLRPC_VectorGetStringWithID(params, "flag");
	
	if ((name && !validate_name(name)) || (flag && !validate_cflag(flag)))
		return method_error(MEREQ);
	
	if (name) {
		if (vxdb_getxid(name, &xid) == -1)
			return method_error(MENOENT);
		
		if (flag)
			dbr = dbi_conn_queryf(vxdb,
				"SELECT flag FROM vx_flags WHERE xid = %d AND flag = '%s'",
				xid, flag);
		
		else
			dbr = dbi_conn_queryf(vxdb,
				"SELECT flag FROM vx_flags WHERE xid = %d",
				xid);
		
		if (!dbr)
			return method_error(MEVXDB);
		
		while (dbi_result_next_row(dbr)) {
			XMLRPC_AddValueToVector(response,
				XMLRPC_CreateValueString(NULL, dbi_result_get_string(dbr, "flag"), 0));
		}
	}
	
	else {
		int i;
		
		for (i = 0; cflags_list[i].key; i++)
			XMLRPC_AddValueToVector(response,
				XMLRPC_CreateValueString(NULL, cflags_list[i].key, 0));
	}
	
	return response;
}
