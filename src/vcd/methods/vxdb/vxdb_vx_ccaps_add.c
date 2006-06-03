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

#include <string.h>

#include "lucid.h"
#include "xmlrpc.h"

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vxdb.vx.ccaps.add(string name, string ccap) */
XMLRPC_VALUE m_vxdb_vx_ccaps_add(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	const char *name  = XMLRPC_VectorGetStringWithID(params, "name");
	const char *_ccap = XMLRPC_VectorGetStringWithID(params, "ccap");
	
	char ccap[128];
	
	if (_ccap)
		strncpy(ccap, _ccap, 128);
	
	if (!validate_name(name) || !validate_ccap(str_toupper(ccap)))
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	dbr = dbi_conn_queryf(vxdb,
		"INSERT OR REPLACE INTO vx_ccaps (xid, ccap) VALUES (%d, '%s')",
		xid, ccap);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	return NULL;
}
