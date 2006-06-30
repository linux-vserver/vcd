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

/* vxdb.dx.limit.set(string name, string path, int space, int inodes, int reserved) */
xmlrpc_value *m_vxdb_dx_limit_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	const char *name, *path;
	int space, inodes, reserved;
	xid_t xid;
	dbi_result dbr;
	
	params = method_init(env, p, VCD_CAP_DLIM, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,s:i,s:i,s:i,*}",
		"name", &name,
		"path", &path,
		"space", &space,
		"inodes", &inodes,
		"reserved", &reserved);
	method_return_if_fault(env);
	
	if (!validate_name(name) || !validate_path(path) ||
	    !validate_dlimits(space, inodes, reserved))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	dbr = dbi_conn_queryf(vxdb,
		"INSERT OR REPLACE INTO dx_limit (xid, path, space, inodes, reserved) "
		"VALUES (%d, '%s', %d, %d, %d)",
		xid, path, space, inodes, reserved);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	return params;
}
