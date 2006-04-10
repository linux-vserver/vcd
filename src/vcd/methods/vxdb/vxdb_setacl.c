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

#include "confuse.h"
#include "xmlrpc.h"

#include "auth.h"
#include "log.h"
#include "vxdb.h"

XMLRPC_VALUE m_vxdb_setacl(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE request, auth, params;
	XMLRPC_VALUE response;
	char *username, *acl_read, *acl_write;
	
	SDBM *db;
	DATUM k, v;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	params  = XMLRPC_VectorNext(request);
	
	if (!auth_capable(auth, "vxdb.getacl"))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	username  = (char *) XMLRPC_VectorGetStringWithID(params, "username");
	acl_read  = (char *) XMLRPC_VectorGetStringWithID(params, "acl_read");
	acl_write = (char *) XMLRPC_VectorGetStringWithID(params, "acl_write");
	
	if (!username)
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	mkdir(__LOCALSTATEDIR "/vxdb", 0600);
	
	if (acl_read) {
		db = sdbm_open(__LOCALSTATEDIR "/vxdb/acl_read", O_RDWR|O_CREAT, 0600);
		
		if (db == NULL) {
			LOGPWARN("sdbm_open");
			return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
		}
		
		k.dptr  = username;
		k.dsize = strlen(k.dptr);
		
		v.dptr  = acl_read;
		v.dsize = strlen(v.dptr);
		
		if (sdbm_store(db, k, v, SDBM_REPLACE) == -1) {
			sdbm_close(db);
			return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
		}
		
		sdbm_close(db);
	}
	
	if (acl_write) {
		db = sdbm_open(__LOCALSTATEDIR "/vxdb/acl_write", O_RDWR|O_CREAT, 0600);
		
		if (db == NULL) {
			LOGPWARN("sdbm_open");
			return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
		}
		
		k.dptr  = username;
		k.dsize = strlen(k.dptr);
		
		v.dptr  = acl_write;
		v.dsize = strlen(v.dptr);
		
		if (sdbm_store(db, k, v, SDBM_REPLACE) == -1) {
			sdbm_close(db);
			return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
		}
		
		sdbm_close(db);
	}
	
	response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("username", username,  0));
	
	return response;
}
