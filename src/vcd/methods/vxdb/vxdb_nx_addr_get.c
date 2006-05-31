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

#include "xmlrpc.h"

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vxdb.nx.addr.get(string name[, string addr]) */
XMLRPC_VALUE m_vxdb_nx_addr_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	XMLRPC_VALUE response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name = XMLRPC_VectorGetStringWithID(params, "name");
	char *addr = XMLRPC_VectorGetStringWithID(params, "addr");
	
	if (!validate_name(name) || (addr && !validate_addr(addr)))
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	if (addr) {
		dbr = dbi_conn_queryf(vxdb,
			"SELECT broadcast,netmask FROM nx_addr "
			"WHERE xid = %d and addr = '%s'",
			xid, addr);
		
		if (!dbr)
			return method_error(MEVXDB);
		
		if (dbi_result_get_numrows(dbr) < 1)
			return method_error(MENOENT);
		
		char *bcas = (char *) dbi_result_get_string(dbr, "broadcast");
		char *netm = (char *) dbi_result_get_string(dbr, "netmask");
		
		XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("netmask", netm, 0));
		XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("broadcast", bcas, 0));
	}
	
	else {
		dbr = dbi_conn_queryf(vxdb, "SELECT addr FROM nx_addr WHERE xid = %d", xid);
		
		if (!dbr)
			return method_error(MEVXDB);
		
		while (dbi_result_next_row(dbr)) {
			XMLRPC_AddValueToVector(response,
				XMLRPC_CreateValueString(NULL, dbi_result_get_string(dbr, "addr"), 0));
		}
	}
	
	return response;
}
