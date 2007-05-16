// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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

/* vxdb.list([string username]) */
xmlrpc_value *m_vxdb_list(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params, *response = NULL;
	char *curuser, *user;
	int rc, uid = 0;

	method_init(env, p, c, VCD_CAP_INFO, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, p,
			"({s:s,*}{s:s,*})",
			"username", &curuser,
			"username", &user,
			&params);
	method_return_if_fault(env);

	method_empty_params(1, &user);

	if (user && !validate_username(user))
		method_return_faultf(env, MEINVAL,
				"invalid username value: %s", user);

	if (auth_isadmin(curuser)) {
		if (user)
			uid = auth_getuid(user);
	}

	else {
		if (user && !str_equal(curuser, user))
			method_return_faultf(env, MEPERM,
					"cannot list foreign user: %s", user);

		uid = auth_getuid(curuser);
	}

	if (uid)
		rc = vxdb_prepare(&dbr,
				"SELECT xid_name_map.name FROM xid_name_map "
				"INNER JOIN xid_uid_map "
				"ON xid_name_map.xid = xid_uid_map.xid "
				"WHERE xid_uid_map.uid = %d "
				"ORDER BY xid_name_map.name ASC",
				uid);

	else
		rc = vxdb_prepare(&dbr,
				"SELECT name FROM xid_name_map ORDER BY name ASC");

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	response = xmlrpc_array_new(env);

	vxdb_foreach_step(rc, dbr)
		xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"s", vxdb_column_text(dbr, 0)));

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);

	if (xmlrpc_array_size(env, response) < 1)
		method_return_faultf(env, MENOVPS, "%s", "no virtual servers found");
	else
		return response;
}
