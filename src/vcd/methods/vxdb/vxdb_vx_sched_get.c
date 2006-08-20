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

#include "auth.h"
#include "log.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

xmlrpc_value *m_vxdb_vx_sched_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	TRACEIT
	
	xmlrpc_value *params, *response;
	char *name;
	int cpuid;
	xid_t xid;
	vxdb_result *dbr;
	int rc;
	
	params = method_init(env, p, c, VCD_CAP_SCHED, M_OWNER);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:i,*}",
		"name", &name,
		"cpuid", &cpuid);
	method_return_if_fault(env);
	
	if (!validate_name(name))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	if (cpuid == -1)
		rc = vxdb_prepare(&dbr,
			"SELECT cpuid,fillrate,interval,fillrate2,interval2,tokensmin,tokensmax,priobias "
			"FROM vx_sched WHERE xid = %d",
			xid);
	
	else
		rc = vxdb_prepare(&dbr,
			"SELECT cpuid,fillrate,interval,fillrate2,interval2,tokensmin,tokensmax,priobias "
			"FROM vx_sched WHERE xid = %d AND cpuid = %d",
			xid, cpuid);
	
	response = xmlrpc_array_new(env);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else {
		for (rc = vxdb_step(dbr); rc == 1; rc = vxdb_step(dbr))
			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
				"cpuid",     sqlite3_column_int(dbr, 0),
				"fillrate",  sqlite3_column_int(dbr, 1),
				"interval",  sqlite3_column_int(dbr, 2),
				"fillrate2", sqlite3_column_int(dbr, 3),
				"interval2", sqlite3_column_int(dbr, 4),
				"tokensmin", sqlite3_column_int(dbr, 5),
				"tokensmax", sqlite3_column_int(dbr, 6),
				"priobias",  sqlite3_column_int(dbr, 7)));
	}
	
	sqlite3_finalize(dbr);
	
	return response;
}
