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
#include "xmlrpc.h"

#include "log.h"
#include "auth.h"
#include "vxdb.h"

int auth_isvalid(XMLRPC_VALUE auth)
{
	SDBM *db;
	DATUM key, val;
	
	const char *username = XMLRPC_VectorGetStringWithID(auth, "username");
	const char *password = XMLRPC_VectorGetStringWithID(auth, "password");
	
	if (!username || !password)
		return 0;
	
	if ((db = sdbm_open(__LOCALSTATEDIR "/auth_passwd", O_RDONLY, 0)) == NULL) {
		LOGPWARN("sdbm_open");
		return 0;
	}
	
	key.dptr  = (char *) username;
	key.dsize = strlen(key.dptr);
	
	val = sdbm_fetch(db, key);
	
	sdbm_close(db);
	
	if (val.dsize > 0 && strncmp(password, val.dptr, val.dsize) == 0)
		return 1;
	
	return 0;
}

int auth_capable(XMLRPC_VALUE auth, uint64_t cap)
{
	SDBM *db;
	DATUM key, val;
	
	uint64_t flags;
	
	const char *username = XMLRPC_VectorGetStringWithID(auth, "username");
	const char *password = XMLRPC_VectorGetStringWithID(auth, "password");
	
	if (!username || !password)
		return 0;
	
	if ((db = sdbm_open(__LOCALSTATEDIR "/auth_acl", O_RDONLY, 0)) == NULL) {
		LOGPWARN("sdbm_open");
		return 0;
	}
	
	key.dptr  = (char *) username;
	key.dsize = strlen(key.dptr);
	
	val = sdbm_fetch(db, key);
	
	sdbm_close(db);
	
	if (val.dsize > 0) {
		flags = strtoull(val.dptr, NULL, 10);
		
		if (flags & cap || flags & VCD_CAP_ADMIN)
			return 1;
	}
	
	return 0;
}

int auth_vxowner(XMLRPC_VALUE auth, char *name)
{
	SDBM *db;
	DATUM key, val;
	
	char *db_file;
	
	const char *username = XMLRPC_VectorGetStringWithID(auth, "username");
	const char *password = XMLRPC_VectorGetStringWithID(auth, "password");
	
	if (!username || !password)
		return 0;
	
	if (auth_capable(auth, VCD_CAP_ADMIN))
		return 1;
	
	if (strcmp(username, vxdb_get(name, "vx.owner")) == 0)
		return 1;
	
	return 0;
}
