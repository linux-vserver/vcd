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

#include "xmlrpc.h"

#include "auth.h"
#include "vxdb.h"

XMLRPC_VALUE m_vxdb_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE request, auth, params;
	XMLRPC_VALUE response;
	char *name, *key, *val;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	params  = XMLRPC_VectorNext(request);
	
	if (!auth_isvalid(auth))
		return XMLRPC_UtilityCreateFault(401, "Unauthorized");
	
	if (!auth_capable(auth, VCD_CAP_VXDB_SET))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	name = (char *) XMLRPC_VectorGetStringWithID(params, "name");
	key  = (char *) XMLRPC_VectorGetStringWithID(params, "key");
	val  = (char *) XMLRPC_VectorGetStringWithID(params, "value");
	
	if (!name || !key || !val)
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	if (!auth_vxowner(auth, name))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	if (!vxdb_validkey(key))
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	if (vxdb_set(name, key, val) == -1)
		return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	
	if ((val = vxdb_get(name, key)) == NULL)
		return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	
	response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	XMLRPC_AddValuesToVector(response,
	                         XMLRPC_CreateValueString("name", name, 0),
	                         XMLRPC_CreateValueString("key",  key,  0),
	                         XMLRPC_CreateValueString("val",  val,  0));
	
	return response;
}
