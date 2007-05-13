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

/* vxdb.owner.remove(string name[, string username]) */
xmlrpc_value *m_vxdb_owner_remove(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name, *user;
	xid_t xid;
	int uid = 0, rc;

	params = method_init(env, p, c, VCD_CAP_AUTH, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,*}",
			"name", &name,
			"username", &user);
	method_return_if_fault(env);

	method_empty_params(1, &user);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	if (user && !(uid = auth_getuid(user)))
		method_return_faultf(env, MENOUSER,
				"user does not exist: %s", user);

	if (uid)
		rc = vxdb_exec(
				"DELETE FROM xid_uid_map WHERE xid = %d AND uid = %d",
				xid, uid);

	else
		rc = vxdb_exec(
				"DELETE FROM xid_uid_map WHERE xid = %d",
				xid);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	return xmlrpc_nil_new(env);
}
