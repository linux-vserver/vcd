// Copyright 2007 Luca Longinotti <chtekk@gentoo.org>
//           2007 Benedikt Böhm <hollow@gentoo.org>
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
#include <lucid/str.h>

/* vg.add(string group, string name) */
xmlrpc_value *m_vg_add(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *group, *name;
	int rc, gid = 0;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_AUTH, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,*}",
			"group", &group,
			"name",  &name);
	method_return_if_fault(env);

	if (!validate_group(group))
		method_return_faultf(env, MEINVAL,
				"invalid group value: %s", group);

	if (str_equal(group, "all"))
		return xmlrpc_nil_new(env);

	if (!(gid = vxdb_getgid(group))) {
		rc = vxdb_prepare(&dbr,
				"SELECT gid FROM groups ORDER BY gid DESC LIMIT 1");

		if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
			gid = vxdb_column_int(dbr, 0) + 1;
		else
			gid = 1;

		vxdb_finalize(dbr);

		rc = vxdb_exec(
				"INSERT INTO groups (gid, name) "
				"VALUES (%d, '%s')",
				gid, group);

		if (rc != VXDB_OK)
			method_return_vxdb_fault(env);
	}

	if (!str_isempty(name)) {
		if (!validate_name(name))
			method_return_faultf(env, MEINVAL,
					"invalid name value: %s", name);

		if (!(xid = vxdb_getxid(name)))
			method_return_fault(env, MENOVPS);

		rc = vxdb_exec(
				"INSERT INTO xid_gid_map (xid, gid) "
				"VALUES (%d, %d)",
				xid, gid);

		if (rc != VXDB_OK)
			method_return_vxdb_fault(env);
	}

	return xmlrpc_nil_new(env);
}
