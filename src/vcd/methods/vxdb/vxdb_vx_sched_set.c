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

#include <lucid/log.h>

xmlrpc_value *m_vxdb_vx_sched_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name;
	int cpuid, fillrate, interval, fillrate2, interval2;
	int tokensmin, tokensmax, rc;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_SCHED, M_OWNER|M_LOCK);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:i,s:i,s:i,s:i,s:i,s:i,s:i,*}",
			"name", &name,
			"cpuid", &cpuid,
			"fillrate", &fillrate,
			"interval", &interval,
			"fillrate2", &fillrate2,
			"interval2", &interval2,
			"tokensmin", &tokensmin,
			"tokensmax", &tokensmax);
	method_return_if_fault(env);

	if (!validate_cpuid(cpuid))
		method_return_faultf(env, MEINVAL,
				"cpuid out of range: %d", cpuid);

	if (!validate_token_bucket(fillrate, interval,
	                           fillrate2, interval2,
	                           tokensmin, tokensmax))
		method_return_faultf(env, MEINVAL,
				"invalid bucket specification: %d,%d,%d,%d,%d,%d",
				fillrate, interval, fillrate2,
				interval2, tokensmin, tokensmax);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	rc = vxdb_exec(
			"INSERT OR REPLACE INTO vx_sched "
			"(xid, fillrate, interval, fillrate2, "
			"interval2, tokensmin, tokensmax, cpuid) "
			"VALUES (%d, %d, %d, %d, %d, %d, %d, %d)",
			xid, fillrate, interval, fillrate2, interval2,
			tokensmin, tokensmax, cpuid);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	return xmlrpc_nil_new(env);
}
