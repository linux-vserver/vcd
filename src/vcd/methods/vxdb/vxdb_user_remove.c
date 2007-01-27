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
#include <lucid/log.h>
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

xmlrpc_value *m_vxdb_user_remove(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *user;
	int uid, rc;

	params = method_init(env, p, c, VCD_CAP_AUTH, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
		"{s:s,*}",
		"username", &user);
	method_return_if_fault(env);

	if (!validate_username(user))
		method_return_fault(env, MEINVAL);

	if ((uid = auth_getuid(user)) == 0)
		method_return_fault(env, MENOUSER);

	rc = vxdb_exec(
		"BEGIN TRANSACTION;"
		"DELETE FROM xid_uid_map WHERE uid = %1$d;"
		"DELETE FROM user_caps WHERE uid = %1$d;"
		"DELETE FROM user WHERE uid = %1$d;"
		"COMMIT TRANSACTION;",
		uid);

	if (rc)
		method_return_fault(env, MEVXDB);

	return xmlrpc_nil_new(env);
}
