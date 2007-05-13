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

/* vxdb.mount.get(string name[, string dst]) */
xmlrpc_value *m_vxdb_mount_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params, *response = NULL;
	char *name, *dst;
	xid_t xid;
	int rc;

	params = method_init(env, p, c, VCD_CAP_MOUNT, M_OWNER);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,*}",
			"name", &name,
			"dst", &dst);
	method_return_if_fault(env);

	method_empty_params(1, &dst);

	if (dst && !validate_path(dst))
		method_return_faultf(env, MEINVAL,
				"invalid dst value: %s", dst);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	if (dst)
		rc = vxdb_prepare(&dbr,
				"SELECT src,dst,type,opts FROM mount "
				"WHERE xid = %d AND dst = '%s'",
				xid, dst);

	else
		rc = vxdb_prepare(&dbr,
				"SELECT src,dst,type,opts FROM mount "
				"WHERE xid = %d ORDER BY dst ASC",
				xid);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	response = xmlrpc_array_new(env);

	vxdb_foreach_step(rc, dbr)
		xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:s,s:s,s:s}",
				"src",  vxdb_column_text(dbr, 0),
				"dst",  vxdb_column_text(dbr, 1),
				"type", vxdb_column_text(dbr, 2),
				"opts", vxdb_column_text(dbr, 3)));

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);

	return response;
}
