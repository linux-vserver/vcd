// Copyright 2007 Luca Longinotti <chtekk@gentoo.org>
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

/* vxdb.vx.cpuset.get(string name) */
xmlrpc_value *m_vxdb_vx_cpuset_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params, *response = NULL;
	char *name;
	int rc;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_SCHED, M_OWNER);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,*}",
			"name",  &name);
	method_return_if_fault(env);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	rc = vxdb_prepare(&dbr,
			"SELECT cpus,mems,virtualize "
			"FROM vx_cpuset WHERE xid = %d",
			xid);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	rc = vxdb_step(dbr);

	if (rc == VXDB_ROW)
		response = xmlrpc_build_value(env,
				"{s:s,s:s,s:i}",
				"cpus",       vxdb_column_text(dbr, 0),
				"mems",       vxdb_column_text(dbr, 1),
				"virtualize", vxdb_column_int(dbr, 2));
	else if (rc == VXDB_DONE)
		response = xmlrpc_build_value(env,
				"{s:s,s:s,s:i}",
				"cpus",       "",
				"mems",       "",
				"virtualize", 0);
	else
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);

	return response;
}
