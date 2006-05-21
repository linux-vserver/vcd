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

/* vxdb.init.method.set(string name,
     [string method[, string stop[, string start[,int timeout]]]]) */
XMLRPC_VALUE m_vxdb_init_method_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name   = XMLRPC_VectorGetStringWithID(params, "name");
	char *method = XMLRPC_VectorGetStringWithID(params, "method");
	char *start  = XMLRPC_VectorGetStringWithID(params, "start");
	char *stop   = XMLRPC_VectorGetStringWithID(params, "stop");
	int timeout  = XMLRPC_VectorGetIntWithID(params, "timeout");
	
	if (!name)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT xid FROM init_method WHERE xid = %d",
		xid);
	
	if (dbr && dbi_result_get_numrows(dbr) > 0) {
		if (method)
			dbr = dbi_conn_queryf(vxdb,
				"UPDATE init_method SET method = '%s' WHERE xid = %d",
				method, xid);
		
		if (start)
			dbr = dbi_conn_queryf(vxdb,
				"UPDATE init_method SET start = '%s' WHERE xid = %d",
				start, xid);
		
		if (stop)
			dbr = dbi_conn_queryf(vxdb,
				"UPDATE init_method SET stop = '%s' WHERE xid = %d",
				stop, xid);
		
		if (timeout > 0)
			dbr = dbi_conn_queryf(vxdb,
				"UPDATE init_method SET timeout = %d WHERE xid = %d",
				timeout, xid);
	}
	
	else if (dbr && dbi_result_get_numrows(dbr) == 0) {
		if (!method || !*method)
			method = "init";
		
		if (!start)
			start = "";
		
		if (!stop)
			stop = "";
		
		dbr = dbi_conn_queryf(vxdb,
			"INSERT INTO init_method (xid, method, start, stop, timeout) "
			"VALUES (%d, '%s', '%s', '%s', %d)",
			xid, method, start, stop, timeout);
	}
	
	else
		return method_error(MEVXDB);
	
	return params;
}
