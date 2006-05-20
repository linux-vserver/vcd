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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "xmlrpc.h"

#include "auth.h"
#include "lists.h"
#include "methods.h"
#include "vxdb.h"

/* vxdb.vx.flags.add(string name, string flag) */
XMLRPC_VALUE m_vxdb_vx_flags_add(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name = XMLRPC_VectorGetStringWithID(params, "name");
	char *flag = XMLRPC_VectorGetStringWithID(params, "flag");
	
	if (!name || !flag || flist64_getval(cflags_list, flag, NULL) == -1)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT xid FROM vx_flags WHERE xid = %d AND flag = '%s'",
		xid, flag);
	
	if (dbi_result_get_numrows(dbr) != 0)
		return method_error(MEEXIST);
	
	dbr = dbi_conn_queryf(vxdb,
		"INSERT INTO vx_flags (xid, flag) VALUES (%d, '%s')",
		xid, flag);
	
	if (!dbr)
		return method_error(MEVXDB);
		
	return params;
}
