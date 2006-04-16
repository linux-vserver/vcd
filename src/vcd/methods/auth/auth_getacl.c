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

#include "pathconfig.h"

#include <string.h>
#include <fcntl.h>

#include "lucid.h"
#include "xmlrpc.h"

#include "auth.h"
#include "log.h"

XMLRPC_VALUE m_auth_getacl(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE request, auth, params;
	XMLRPC_VALUE response, methods;
	char *username, *acl = NULL, *p;
	
	SDBM *db;
	DATUM k, v;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	params  = XMLRPC_VectorNext(request);
	
	if (!auth_capable(auth, "auth.getacl"))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	username = (char *) XMLRPC_VectorGetStringWithID(params, "username");
	
	if (!username)
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	if (!auth_exists(username))
		return XMLRPC_UtilityCreateFault(404, "Not Found");
	
	if (!(db = sdbm_open(__LOCALSTATEDIR "/auth/acl", O_RDONLY, 0))) {
		log_warn("sdbm_open: %s", strerror(errno));
		return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	}
	
	k.dptr  = username;
	k.dsize = strlen(k.dptr);
	
	v = sdbm_fetch(db, k);
	
	if (v.dsize > 0)
		acl = strndup(v.dptr, v.dsize);
	
	sdbm_close(db);
	
	response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	methods  = XMLRPC_CreateVector("methods", xmlrpc_vector_array);
	
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("username", username, 0));
	
	while ((p = strsep(&acl, ",")) != NULL)
		XMLRPC_AddValueToVector(methods, XMLRPC_CreateValueString(NULL, p, 0));
	
	XMLRPC_AddValueToVector(response, methods);
	
	return response;
}