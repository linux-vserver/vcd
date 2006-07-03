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
xmlrpc_value *m_vxdb_init_method_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	char *name, *method, *start, *stop;
	int timeout;
	xid_t xid;
	dbi_result dbr;
	
	params = method_init(env, p, VCD_CAP_INIT, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,s:s,s:s,s:i,*}",
		"name", &name,
		"method", &method,
		"start", &start,
		"stop", &stop,
		"timeout", &timeout);
	method_return_if_fault(env);
	
	method_empty_params(2, &start, &stop);
	
	if (!validate_name(name) || !validate_init_method(method) ||
	   (start && !validate_runlevel(start)) ||
	   (stop  && !validate_runlevel(stop)))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	dbr = dbi_conn_queryf(vxdb, "BEGIN TRANSACTION");
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT stop,start,timeout FROM init_method WHERE xid = %d",
		xid);
	
	if (!dbr)
		goto rollback;
	
	if (dbi_result_get_numrows(dbr) > 0) {
		dbi_result_first_row(dbr);
		
		if (!start)
			start = (char *) dbi_result_get_string(dbr, "start");
		
		if (!stop)
			stop = (char *) dbi_result_get_string(dbr, "stop");
		
		if (timeout < 1)
			timeout = dbi_result_get_int(dbr, "timeout");
	}
	
	else {
		if (!start)
			start = "";
		
		if (!stop)
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
	
	return xmlrpc_nil_new(env);
	
rollback:
	dbi_conn_queryf(vxdb, "ROLLBACK TRANSACTION");
	method_return_fault(env, MEVXDB);
}
