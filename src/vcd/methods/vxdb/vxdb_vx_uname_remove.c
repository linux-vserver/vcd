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

#include "xmlrpc.h"

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vxdb.vx.uname.remove(string name[, string uname]) */
XMLRPC_VALUE m_vxdb_vx_uname_remove(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	const char *name  = XMLRPC_VectorGetStringWithID(params, "name");
	const char *uname = XMLRPC_VectorGetStringWithID(params, "uname");
	
	if (!validate_name(name) || (uname && !validate_uname(uname)))
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	if (uname)
		dbr = dbi_conn_queryf(vxdb,
			"DELETE FROM vx_uname WHERE xid = %d AND uname = '%s'",
			xid, uname);
	
	else
		dbr = dbi_conn_queryf(vxdb,
			"DELETE FROM vx_uname WHERE xid = %d",
			xid);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	return NULL;
}
