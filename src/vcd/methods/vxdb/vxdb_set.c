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
#include "vxdb.h"

XMLRPC_VALUE m_vxdb_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE request, auth, params;
	XMLRPC_VALUE response;
	XMLRPC_VALUE val;
	char *name, *key, *title, *buf, *p;
	cfg_t *vxdb, *sec;
	cfg_opt_t *opt;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	params  = XMLRPC_VectorNext(request);
	
	if (!auth_capable(auth, "vxdb.set"))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	name  = (char *) XMLRPC_VectorGetStringWithID(params, "name");
	key   = (char *) XMLRPC_VectorGetStringWithID(params, "key");
	title = (char *) XMLRPC_VectorGetStringWithID(params, "title");
	val   =          XMLRPC_VectorGetValueWithID(params,  "value");
	
	if (!name || !key || !val)
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	if (!vxdb_capable_write(auth, name, key))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	if ((vxdb = vxdb_open(name)) == NULL)
		return XMLRPC_UtilityCreateFault(404, "Not Found");
	
	if ((opt = vxdb_lookup(vxdb, key, title)) == NULL) {
		if (vxdb_addsec(vxdb, key, title) == -1)
			return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
		
		if ((opt = vxdb_lookup(vxdb, key, title)) == NULL)
			return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	}
	
	response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("name", name, 0));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("key", key, 0));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("title", title,  0));
	
	switch (XMLRPC_GetValueType(val)) {
	case xmlrpc_boolean:
		cfg_opt_setnbool(opt, XMLRPC_GetValueBoolean(val), 0);
		break;
	
	case xmlrpc_double:
		cfg_opt_setnfloat(opt, XMLRPC_GetValueDouble(val), 0);
		break;
	
	case xmlrpc_int:
		cfg_opt_setnint(opt, XMLRPC_GetValueInt(val), 0);
		break;
	
	case xmlrpc_string:
		cfg_opt_setnstr(opt, XMLRPC_GetValueString(val), 0);
		break;
	
	default:
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	}
	
	if (vxdb_closewrite(vxdb) == -1)
		return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	
	return response;
}
