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

#include "lucid.h"
#include "xmlrpc.h"

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vxdb.init.method.set(string name,
     string method[, string stop[, string start[,int timeout]]]) */
XMLRPC_VALUE m_vxdb_init_method_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	const char *name   = XMLRPC_VectorGetStringWithID(params, "name");
	const char *method = XMLRPC_VectorGetStringWithID(params, "method");
	const char *start  = XMLRPC_VectorGetStringWithID(params, "start");
	const char *stop   = XMLRPC_VectorGetStringWithID(params, "stop");
	int timeout  = XMLRPC_VectorGetIntWithID(params, "timeout");
	
	if (!validate_name(name) || !validate_init_method(method) ||
	   (start && !validate_runlevel(start)) ||
	   (stop  && !validate_runlevel(stop)))
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	dbr = dbi_conn_queryf(vxdb, "BEGIN TRANSACTION");
	
	if (!dbr)
		return method_error(MEVXDB);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT stop,start,timeout FROM init_method WHERE xid = %d",
		xid);
	
	if (!dbr)
		goto rollback;
	
	if (dbi_result_get_numrows(dbr) > 0) {
		dbi_result_first_row(dbr);
		
		if (str_isempty(start))
			start = dbi_result_get_string(dbr, "start");
		
		if (str_isempty(stop))
			stop = dbi_result_get_string(dbr, "stop");
		
		if (timeout < 1)
			timeout = dbi_result_get_int(dbr, "timeout");
	}
	
	else {
		if (str_isempty(start))
			start = "";
		
		if (str_isempty(stop))
			stop = "";
		
		if (timeout < 1)
			timeout = 30;
	}
	
	dbr = dbi_conn_queryf(vxdb,
		"INSERT OR REPLACE INTO init_method (xid, method, start, stop, timeout) "
		"VALUES (%d, '%s', '%s', '%s', %d)",
		xid, method, start, stop, timeout);
	
	if (!dbr)
		goto rollback;
	
	dbr = dbi_conn_queryf(vxdb, "COMMIT TRANSACTION");
	
	if (!dbr)
		goto rollback;
	
	return NULL;
	
rollback:
	dbi_conn_queryf(vxdb, "ROLLBACK TRANSACTION");
	return method_error(MEVXDB);
}
