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
#include "vxdb.h"

#include <lucid/log.h>

xmlrpc_value *m_vxdb_vx_sched_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params, *response = NULL;
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

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	/* -2 is used to get all configured cpus, we can't use -1 here,
	 * since it has special meaning in helper.startup */
	if (cpuid == -2)
		rc = vxdb_prepare(&dbr,
				"SELECT cpuid, fillrate, interval, fillrate2, "
				"interval2, tokensmin, tokensmax "
				"FROM vx_sched WHERE xid = %d",
				xid);

	else
		rc = vxdb_prepare(&dbr,
				"SELECT cpuid, fillrate, interval, fillrate2, "
				"interval2, tokensmin, tokensmax "
				"FROM vx_sched WHERE xid = %d AND cpuid = %d",
				xid, cpuid);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	response = xmlrpc_array_new(env);

	vxdb_foreach_step(rc, dbr)
		xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
				"cpuid",     vxdb_column_int(dbr, 0),
				"fillrate",  vxdb_column_int(dbr, 1),
				"interval",  vxdb_column_int(dbr, 2),
				"fillrate2", vxdb_column_int(dbr, 3),
				"interval2", vxdb_column_int(dbr, 4),
				"tokensmin", vxdb_column_int(dbr, 5),
				"tokensmax", vxdb_column_int(dbr, 6)));

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);

	return response;
}
