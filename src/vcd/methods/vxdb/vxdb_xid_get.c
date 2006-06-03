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

/* vxdb.xid.get(string name) */
XMLRPC_VALUE m_vxdb_xid_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE params   = method_get_params(r);
	XMLRPC_VALUE response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	const char *name = XMLRPC_VectorGetStringWithID(params, "name");
	
	if (!validate_name(name))
		return method_error(MEREQ);
	
	xid_t xid;
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MEVXDB);
	
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("xid", xid));
	
	return response;
}
