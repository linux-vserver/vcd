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

#include "confuse.h"
#include "xmlrpc.h"

#include "auth.h"
#include "vxdb.h"

XMLRPC_VALUE m_vxdb_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE request, auth, params;
	XMLRPC_VALUE response;
	char *name, *key, *title;
	cfg_t *vxdb;
	cfg_opt_t *opt;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	params  = XMLRPC_VectorNext(request);
	
	if (!auth_capable(auth, "vxdb.get"))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	name  = (char *) XMLRPC_VectorGetStringWithID(params, "name");
	key   = (char *) XMLRPC_VectorGetStringWithID(params, "key");
	title = (char *) XMLRPC_VectorGetStringWithID(params, "title");
	
	if (!name || !key)
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	if (!vxdb_capable_read(auth, name, key))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	if ((vxdb = vxdb_open(name)) == NULL ||
	    (opt  = vxdb_lookup(vxdb, key, title)) == NULL)
		return XMLRPC_UtilityCreateFault(404, "Not Found");
	
	response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("name", name, 0));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("key",  key, 0));
	
	switch (opt->type) {
	case CFGT_INT:
		XMLRPC_AddValueToVector(response,
		                        XMLRPC_CreateValueInt("value", cfg_opt_getnint(opt, 0)));
		break;
	
	case CFGT_FLOAT:
		XMLRPC_AddValueToVector(response,
		                        XMLRPC_CreateValueDouble("value", cfg_opt_getnfloat(opt, 0)));
		break;
	
	case CFGT_STR:
		XMLRPC_AddValueToVector(response,
		                        XMLRPC_CreateValueString("value", cfg_opt_getnstr(opt, 0), 0));
		break;
	
	case CFGT_BOOL:
		XMLRPC_AddValueToVector(response,
		                        XMLRPC_CreateValueBoolean("value", cfg_opt_getnbool(opt, 0)));
		break;
	
	default:
		return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	}
	
	return response;
}
