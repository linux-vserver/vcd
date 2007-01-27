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

xmlrpc_value *m_vxdb_vx_flags_remove(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name, *flag;
	xid_t xid;
	int rc;

	params = method_init(env, p, c, VCD_CAP_CFLAG, M_OWNER|M_LOCK);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,*}",
		"name", &name,
		"flag", &flag);
	method_return_if_fault(env);

	method_empty_params(1, &flag);

	if (!validate_name(name) || (flag && !validate_cflag(flag)))
		method_return_fault(env, MEINVAL);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	if (flag)
		rc = vxdb_exec(
			"DELETE FROM vx_flags WHERE xid = %d AND flag = '%s'",
			xid, flag);

	else
		rc = vxdb_exec(
			"DELETE FROM vx_flags WHERE xid = %d",
			xid);

	if (rc)
		method_return_fault(env, MEVXDB);

	return xmlrpc_nil_new(env);
}
