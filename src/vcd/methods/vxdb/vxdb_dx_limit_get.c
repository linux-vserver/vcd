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

xmlrpc_value *m_vxdb_dx_limit_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params, *response = NULL;
	char *name, *path;
	xid_t xid;
	vxdb_result *dbr;
	int rc;
	
	params = method_init(env, p, VCD_CAP_DLIM, M_OWNER);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,*}",
		"name", &name,
		"path", &path);
	method_return_if_fault(env);
	
	method_empty_params(1, &path);
	
	if (!validate_name(name) || (path && !validate_path(path)))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	if (path)
		rc = vxdb_prepare(&dbr,
			"SELECT path,space,inodes,reserved FROM dx_limit "
			"WHERE xid = %d AND path = '%s'",
			xid, path);
	
	else
		rc = vxdb_prepare(&dbr,
			"SELECT path,space,inodes,reserved FROM dx_limit "
			"WHERE xid = %d",
			xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else {
		response = xmlrpc_array_new(env);
		
		vxdb_foreach_step(rc, dbr)
			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:i,s:i,s:i}",
				"path",     sqlite3_column_text(dbr, 0),
				"space",    sqlite3_column_int(dbr, 1),
				"inodes",   sqlite3_column_int(dbr, 2),
				"reserved", sqlite3_column_int(dbr, 3)));
		
		if (rc == -1)
			method_set_fault(env, MEVXDB);
	}
	
	sqlite3_finalize(dbr);
	
	return response;
}
