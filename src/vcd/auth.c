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

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "lucid.h"
#include "confuse.h"
#include "xmlrpc.h"

#include "log.h"
#include "auth.h"

extern cfg_t *cfg;

int auth_isvalid(XMLRPC_VALUE auth)
{
	SDBM *db;
	DATUM k, v;
	
	char *username = (char *) XMLRPC_VectorGetStringWithID(auth, "username");
	char *password = (char *) XMLRPC_VectorGetStringWithID(auth, "password");
	
	if (!username || !password)
		return 0;
	
	db = sdbm_open(__LOCALSTATEDIR "/auth/passwd", O_RDONLY, 0);
	
	if (db == NULL) {
		LOGPWARN("sdbm_open");
		return 0;
	}
	
	k.dptr  = username;
	k.dsize = strlen(k.dptr);
	
	v = sdbm_fetch(db, k);
	
	sdbm_close(db);
	
	if (v.dsize > 0 && strncmp(password, v.dptr, v.dsize) == 0)
		return 1;
	
	return 0;
}

int auth_exists(char *username)
{
	SDBM *db;
	DATUM k, v;
	
	if (!username)
		return 0;
	
	db = sdbm_open(__LOCALSTATEDIR "/auth/passwd", O_RDONLY, 0);
	
	if (db == NULL) {
		LOGPWARN("sdbm_open");
		return 0;
	}
	
	k.dptr  = username;
	k.dsize = strlen(k.dptr);
	
	v = sdbm_fetch(db, k);
	
	sdbm_close(db);
	
	if (v.dsize > 0)
		return 1;
	
	return 0;
}

int auth_isuser(XMLRPC_VALUE auth, char *user)
{
	char *username = (char *) XMLRPC_VectorGetStringWithID(auth, "username");
	
	if (!username || !auth_exists(username))
		return 0;
	
	if (strcmp(user, username) == 0)
		return 1;
	
	return 0;
}

int auth_isadmin(XMLRPC_VALUE auth)
{
	int i, m = cfg_size(cfg, "admins");
	
	char *username = (char *) XMLRPC_VectorGetStringWithID(auth, "username");
	
	if (!username || !auth_exists(username))
		return 0;
	
	for (i = 0; i < m; i++) {
		char *admin = cfg_getnstr(cfg, "admins", i);
		
		if (strcmp(username, admin) == 0)
			return 1;
	}
	
	return 0;
}

int auth_capable(XMLRPC_VALUE auth, char *method)
{
	SDBM *db;
	DATUM k, v;
	char *buf, *bufp, *p;
	int rc = 0;
	
	char *username = (char *) XMLRPC_VectorGetStringWithID(auth, "username");
	
	if (!username || !auth_isvalid(auth))
		return 0;
	
	if (auth_isadmin(auth))
		return 1;
	
	db = sdbm_open(__LOCALSTATEDIR "/auth/acl", O_RDONLY, 0);
	
	if (db == NULL) {
		LOGPWARN("sdbm_open");
		return 0;
	}
	
	k.dptr  = username;
	k.dsize = strlen(k.dptr);
	
	v = sdbm_fetch(db, k);
	
	sdbm_close(db);
	
	if (v.dsize > 0) {
		bufp = buf = strndup(v.dptr, v.dsize);
		
		while ((p = strsep(&buf, ",")) != NULL) {
			if (strcmp(method, p) == 0) {
				rc = 1;
				break;
			}
		}
		
		free(bufp);
	}
	
	return rc;
}

int auth_vxowner(XMLRPC_VALUE auth, char *name)
{
	SDBM *db;
	DATUM k, v;
	char *p, *owners, *ownersp;
	
	char *username = (char *) XMLRPC_VectorGetStringWithID(auth, "username");
	
	if (!username || !auth_isvalid(auth))
		return 0;
	
	if (auth_isadmin(auth))
		return 1;
	
	db = sdbm_open(__LOCALSTATEDIR "/maps/owner", O_RDONLY, 0);
	
	if (db == NULL) {
		LOGPWARN("sdbm_open");
		return 0;
	}
	
	k.dptr  = (char *) name;
	k.dsize = strlen(k.dptr);
	
	v = sdbm_fetch(db, k);
	
	sdbm_close(db);
	
	if (v.dsize > 0) {
		ownersp = owners = strndup(v.dptr, v.dsize);
		
		while ((p = strsep(&owners, ",")) != NULL) {
			if (strcmp(username, p) == 0)
				return 1;
		}
		
		free(ownersp);
	}
	
	return 0;
}
