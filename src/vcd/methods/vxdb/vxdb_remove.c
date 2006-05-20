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
#include <vserver.h>

#include "xmlrpc.h"

#include "auth.h"
#include "methods.h"
#include "vxdb.h"

/* vxdb.remove(string name) */
XMLRPC_VALUE m_vxdb_remove(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name = XMLRPC_VectorGetStringWithID(params, "name");
	
	if (!name)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	if (vx_get_info(xid, NULL) == -1)
		return XMLRPC_UtilityCreateFault(302, "Still running");
	
	dbr = dbi_conn_queryf(vxdb,
		"BEGIN EXCLUSIVE TRANSACTION;"
		"DELETE FROM dx_limit WHERE xid = '%1$d';"
		"DELETE FROM init_method WHERE xid = '%1$d';"
		"DELETE FROM init_mount WHERE xid = '%1$d';"
		"DELETE FROM nx_addr WHERE xid = '%1$d';"
		"DELETE FROM vx_bcaps WHERE xid = '%1$d';"
		"DELETE FROM vx_ccaps WHERE xid = '%1$d';"
		"DELETE FROM vx_flags WHERE xid = '%1$d';"
		"DELETE FROM vx_pflags WHERE xid = '%1$d';"
		"DELETE FROM vx_sched WHERE xid = '%1$d';"
		"DELETE FROM vx_uname WHERE xid = '%1$d';"
		"DELETE FROM xid_name_map WHERE xid = '%1$d';"
		"DELETE FROM xid_uid_map WHERE xid = '%1$d';"
		"COMMIT;",
		xid);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	return params;
}
