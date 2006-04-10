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

XMLRPC_VALUE m_auth_userinfo(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE request, auth;
	XMLRPC_VALUE response;
	
	char *username;
	
	SDBM *db;
	DATUM k;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	
	if (!auth_capable(auth, "auth.userinfo"))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	mkdir(__LOCALSTATEDIR "/auth", 0600);
	
	db = sdbm_open(__LOCALSTATEDIR "/auth/passwd", O_RDONLY, 0);
	
	if (db == NULL) {
		LOGPWARN("sdbm_open");
		return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	}
	
	response = XMLRPC_CreateVector(NULL, xmlrpc_vector_array);
	
	for (k = sdbm_firstkey(db); k.dsize > 0; k = sdbm_nextkey(db)) {
		username = strndup(k.dptr, k.dsize);
		XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString(NULL, username, 0));
	}
	
	sdbm_close(db);
	return response;
}
