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

/* vg.list(string group) */
xmlrpc_value *m_vg_list(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params, *response = NULL;
	char *group;
	int rc, gid;

	params = method_init(env, p, c, VCD_CAP_AUTH, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,*}",
			"group", &group);
	method_return_if_fault(env);

	if (str_isempty(group)) {
		rc = vxdb_prepare(&dbr,
				"SELECT name FROM groups ORDER BY name ASC");
	}

	else {
		if (!validate_group(group))
			method_return_faultf(env, MEINVAL,
					"invalid group value: %s", group);

		if (str_equal(group, "all"))
			method_return_faultf(env, MEINVAL,
					"cannot list group '%s', use vxdb.list instead", group);

		if (!(gid = vxdb_getgid(group)))
			method_return_fault(env, MENOVG);

		rc = vxdb_prepare(&dbr,
				"SELECT xid_name_map.name FROM xid_name_map "
				"INNER JOIN xid_gid_map "
				"ON xid_name_map.xid = xid_gid_map.xid "
				"WHERE xid_gid_map.gid = %d "
				"ORDER BY xid_name_map.name ASC",
				gid);
	}

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	response = xmlrpc_array_new(env);

	vxdb_foreach_step(rc, dbr)
		xmlrpc_array_append_item(env, response,
				xmlrpc_build_value(env, "s", vxdb_column_text(dbr, 0)));

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);

	return response;
}
