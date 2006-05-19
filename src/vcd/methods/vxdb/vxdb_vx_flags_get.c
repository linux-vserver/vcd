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

/* vxdb.vx.flags.get([string name]) */
XMLRPC_VALUE m_vxdb_vx_flags_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = get_method_params(r);
	XMLRPC_VALUE response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	XMLRPC_VALUE flags = XMLRPC_CreateVector("flags", xmlrpc_vector_array);
	
	if (!auth_isadmin(r))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	char *name = XMLRPC_VectorGetStringWithID(params, "name");
	
	if (name) {
		if (vxdb_getxid(name, &xid) == -1)
			return XMLRPC_UtilityCreateFault(404, "Not Found");
		
		dbr = dbi_conn_queryf(vxdb,
			"SELECT flag FROM vx_flags WHERE xid = %d",
			xid);
		
		if (!dbr)
			goto out;
		
		while (dbi_result_next_row(dbr)) {
			char *flag = (char *) dbi_result_get_string(dbr, "flag");
			XMLRPC_AddValueToVector(flags, XMLRPC_CreateValueString(NULL, flag, 0));
		}
	}
	
	else {
		int i;
		
		for (i = 0; cflags_list[i].key; i++)
			XMLRPC_AddValueToVector(flags,
				XMLRPC_CreateValueString(NULL, cflags_list[i].key, 0));
	}
	
out:
	XMLRPC_AddValueToVector(response, flags);
	return response;
}
