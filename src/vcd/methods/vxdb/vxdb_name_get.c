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

/* vxdb.name.get(int xid) */
XMLRPC_VALUE m_vxdb_name_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE params   = method_get_params(r);
	XMLRPC_VALUE response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	int xid = XMLRPC_VectorGetIntWithID(params, "xid");
	
	if (!validate_xid(xid))
		return method_error(MEREQ);
	
	char *name;
	
	if (vxdb_getname(xid, &name) == -1)
		return method_error(MEVXDB);
	
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("name", name, 0));
	
	return response;
}
