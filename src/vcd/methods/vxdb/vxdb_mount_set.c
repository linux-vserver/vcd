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
#include "validate.h"
#include "vxdb.h"

xmlrpc_value *m_vxdb_mount_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	char *name, *src, *dst, *type, *opts;
	xid_t xid;
	int rc;

	params = method_init(env, p, c, VCD_CAP_MOUNT, M_OWNER|M_LOCK);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,s:s,s:s,s:s,*}",
			"name", &name,
			"src", &src,
			"dst", &dst,
			"type", &type,
			"opts", &opts);
	method_return_if_fault(env);

	method_empty_params(3, &src, &type, &opts);

	if (!validate_path(dst))
		method_return_fault(env, MEINVAL);

	if (!src && !type)
		method_return_fault(env, MEINVAL);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	if (!src)
		src = "none";

	if (!type)
		type = "auto";

	if (!opts)
		opts = "defaults";

	rc = vxdb_exec(
			"INSERT OR REPLACE INTO mount (xid, src, dst, type, opts) "
			"VALUES (%d, '%s', '%s', '%s', '%s')",
			xid, src, dst, type, opts);

	if (rc != SQLITE_OK)
		method_return_vxdb_fault(env);

	return xmlrpc_nil_new(env);
}
