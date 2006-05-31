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

/* vxdb.vx.sched.get(string name, int cpuid) */
XMLRPC_VALUE m_vxdb_vx_sched_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params   = method_get_params(r);
	XMLRPC_VALUE response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name = XMLRPC_VectorGetStringWithID(params, "name");
	int cpuid  = XMLRPC_VectorGetIntWithID(params, "cpuid");
	
	if (!validate_name(name) || !validate_cpuid(cpuid))
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT * FROM vx_sched WHERE xid = %d",
		xid);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	if (dbi_result_get_numrows(dbr) < 1)
		return NULL;
	
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("fillrate",
		dbi_result_get_int(dbr, "fill_rate")));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("interval",
		dbi_result_get_int(dbr, "interval")));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("fillrate2",
		dbi_result_get_int(dbr, "fill_rate2")));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("interval2",
		dbi_result_get_int(dbr, "interval2")));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("tokensmin",
		dbi_result_get_int(dbr, "tokens_min")));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("tokensmax",
		dbi_result_get_int(dbr, "tokens_max")));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("priobias",
		dbi_result_get_int(dbr, "prio_bias")));
	
	return response;
}
