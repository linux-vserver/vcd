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

#include <stdio.h>

#include "pathconfig.h"

#include <string.h>
#include <fcntl.h>

#include "lucid.h"

#include "log.h"
#include "xid.h"

int xid_byname(char *name, xid_t *xid)
{
	SDBM *db;
	DATUM k, v;
	
	char *buf;
	char *db_file;
	
	db = sdbm_open(__LOCALSTATEDIR "/maps/xid", O_RDONLY, 0);
	
	if (db == NULL) {
		LOGPWARN("sdbm_open");
		return -1;
	}
	
	k.dptr  = name;
	k.dsize = strlen(k.dptr);
	
	v = sdbm_fetch(db, k);
	
	sdbm_close(db);
	
	if (v.dsize > 0) {
		buf = malloc(v.dsize + 1);
		bzero(buf, v.dsize + 1);
		memcpy(buf, v.dptr, v.dsize);
		*xid = atoi(buf);
		free(buf);
	}
	
	return -1;
}

int xid_toname(xid_t xid, char **name)
{
	int rc = -1;
	
	SDBM *db;
	DATUM k, v;
	
	char *buf, *db_file, *xidstr;
	
	db = sdbm_open(__LOCALSTATEDIR "/maps/xid", O_RDONLY, 0);
	
	if (db == NULL) {
		LOGPWARN("sdbm_open");
		return -1;
	}
	
	asprintf(&xidstr, "%ul", xid);
	
	for (k = sdbm_firstkey(db); k.dsize > 0; k = sdbm_nextkey(db)) {
		v = sdbm_fetch(db, k);
		
		if (strncmp(xidstr, v.dptr, v.dsize) == 0) {
			buf = malloc(v.dsize + 1);
			bzero(buf, v.dsize + 1);
			memcpy(buf, v.dptr, v.dsize);
			*name = buf;
			rc = 0;
			break;
		}
	}
	
	free(xidstr);
	sdbm_close(db);
	
	return rc;
}
