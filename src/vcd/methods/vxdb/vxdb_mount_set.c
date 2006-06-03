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
#include "xmlrpc.h"

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vxdb.mount.set(string name, string path[, string spec[, string opts[, string type]]]) */
XMLRPC_VALUE m_vxdb_mount_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	const char *name = XMLRPC_VectorGetStringWithID(params, "name");
	const char *path = XMLRPC_VectorGetStringWithID(params, "path");
	const char *spec = XMLRPC_VectorGetStringWithID(params, "spec");
	const char *opts = XMLRPC_VectorGetStringWithID(params, "opts");
	const char *type = XMLRPC_VectorGetStringWithID(params, "type");
	
	if (!validate_name(name) || !validate_path(path))
		return method_error(MEREQ);
	
	if (str_isempty(spec) && str_isempty(type))
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	if (str_isempty(spec))
		spec = "none";
	
	if (str_isempty(opts))
		opts = "defaults";
	
	if (str_isempty(type))
		type = "auto";
	
	dbr = dbi_conn_queryf(vxdb,
		"INSERT OR REPLACE INTO mount (xid, spec, file, vfstype, mntops) "
		"VALUES (%d, '%s', '%s', '%s', '%s')",
		xid, spec, path, type, opts);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	return NULL;
}
