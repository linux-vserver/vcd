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
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vxdb.vx.sched.set(string name, int fillrate, int interval,
                  int tokensmin, int tokensmax[,
                  int fillrate2[, int interval2[,
                  int priobias[, int cpuid]]]])
*/
xmlrpc_value *m_vxdb_vx_sched_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	const char *name;
	int cpuid, fillrate, interval, fillrate2, interval2;
	int tokensmin, tokensmax, priobias;
	xid_t xid;
	dbi_result dbr;
	
	params = method_init(env, p, VCD_CAP_SCHED, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,*}",
		"name", &name,
		"cpuid", &cpuid,
		"fillrate", &fillrate,
		"interval", &interval,
		"fillrate2", &fillrate2,
		"interval2", &interval2,
		"tokensmin", &tokensmin,
		"tokensmax", &tokensmax,
		"priobias", &priobias);
	method_return_if_fault(env);
	
	if (!validate_name(name) || !validate_cpuid(cpuid))
		method_return_fault(env, MEINVAL);
	
	if (!validate_token_bucket(fillrate, interval,
	                           fillrate2, interval2,
	                           tokensmin, tokensmax, priobias))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	dbr = dbi_conn_queryf(vxdb,
		"INSERT OR REPLACE INTO vx_sched (xid, fillrate, interval, fillrate2, "
		"interval2, tokensmin, tokensmax, priobias, cpuid) "
		"VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d)",
		xid, fillrate, interval, fillrate2, interval2,
		tokensmin, tokensmax, priobias, cpuid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	return params;
}
