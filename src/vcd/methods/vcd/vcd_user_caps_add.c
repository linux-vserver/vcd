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

xmlrpc_value *m_vcd_user_caps_add(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *user, *cap;
	int uid;
	int rc;

	params = method_init(env, p, c, VCD_CAP_AUTH, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,*}",
			"username", &user,
			"cap", &cap);
	method_return_if_fault(env);

	if (!validate_vcd_cap(cap))
		method_return_faultf(env, MEINVAL,
				"invalid cap value: %s", cap);

	if (!(uid = auth_getuid(user)))
		method_return_fault(env, MENOUSER);

	rc = vxdb_exec(
			"INSERT OR REPLACE INTO user_caps (uid, cap) VALUES (%d, '%s')",
			uid, cap);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	return xmlrpc_nil_new(env);
}
