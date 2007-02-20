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
#include "validate.h"
#include "vxdb.h"

#include <lucid/log.h>
#include <lucid/str.h>

xmlrpc_value *m_vg_remove(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name, *group;
	int gid, rc;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_AUTH, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,*}",
			"group", &group,
			"name", &name);
	method_return_if_fault(env);

	if (!validate_group(group))
		method_return_faultf(env, MEINVAL,
				"invalid group value: %s", group);

	if (str_equal(group, "all"))
		method_return_faultf(env, MEINVAL,
				"cannot remove reserved group '%s'", group);

	if (!(gid = vxdb_getgid(group)))
		method_return_fault(env, MENOVG);

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
	}

	else {
		rc = vxdb_exec(
			"BEGIN EXCLUSIVE TRANSACTION;"
			"DELETE FROM xid_gid_map WHERE gid = %d;"
			"DELETE FROM groups WHERE gid = %d;"
			"COMMIT TRANSACTION;",
			gid, gid);
	}

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	return xmlrpc_nil_new(env);
}
