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

#include "lucid.h"

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vxdb.dx.limit.remove(string name[, string path]) */
xmlrpc_value *m_vxdb_dx_limit_remove(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params, *response;
	const char *name, *path;
	xid_t xid;
	dbi_result dbr;
	
	params = method_init(env, p, VCD_CAP_DLIM, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,*}",
		"name", &name,
		"path", &path);
	method_return_if_fault(env);
	
	if (str_isempty(path))
		path = NULL;
	
	if (!validate_name(name) || (path && !validate_path(path)))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	if (path)
		dbr = dbi_conn_queryf(vxdb,
			"SELECT path,space,inodes,reserved FROM dx_limit "
			"WHERE xid = %d AND path = '%s'",
			xid, path);
	
	else
		dbr = dbi_conn_queryf(vxdb,
			"SELECT path,space,inodes,reserved FROM dx_limit "
			"WHERE xid = %d",
			xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	response = xmlrpc_array_new(env);
	
	while (dbi_result_next_row(dbr))
		xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
			"{s:s,s:i,s:i,s:i}",
			"path",     dbi_result_get_string(dbr, "path"),
			"space",    dbi_result_get_int(dbr, "space"),
			"inodes",   dbi_result_get_int(dbr, "inodes"),
			"reserved", dbi_result_get_int(dbr, "reserved")));
	
	method_return_if_fault(env);
	
	if (path)
		dbr = dbi_conn_queryf(vxdb,
			"DELETE FROM dx_limit WHERE xid = %d AND path = '%s'",
			xid, path);
	
	else
		dbr = dbi_conn_queryf(vxdb,
			"DELETE FROM dx_limit WHERE xid = %d",
			xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	return response;
}
