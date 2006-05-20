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

/* vxdb.vx.uname.get(string name, string field) */
XMLRPC_VALUE m_vxdb_vx_uname_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	XMLRPC_VALUE response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name  = XMLRPC_VectorGetStringWithID(params, "name");
	char *field = XMLRPC_VectorGetStringWithID(params, "field");
	
	if (!name || !field || flist32_getval(vhiname_list, field, NULL) == -1)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT %s FROM vx_uname WHERE xid = %d",
		field, xid);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("name", name, 0));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("field", field, 0));
	
	if (dbi_result_get_numrows(dbr) > 0) {
		dbi_result_first_row(dbr);
		char *value = (char *) dbi_result_get_string(dbr, field);
		XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("value", name, 0));
	}
	
	return response;
}
