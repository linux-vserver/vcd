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
#include "vxdb.h"

static char *valid_keys[] = {
	"vx.id",
	"vx.owner",
	NULL
};

int vxdb_validkey(char *key)
{
	int i;
	
	for (i = 0; valid_keys[i]; i++) {
		if (strcmp(key, valid_keys[i]) == 0)
			return 1;
	}
	
	return 0;
}

char *vxdb_get(char *name, char *key)
{
	SDBM *db;
	DATUM k, v;
	
	char *buf;
	char *db_file;
	
	if (!vxdb_validkey(key))
		return errno = EINVAL, NULL;
	
	asprintf(&db_file, "%s/vx/%s", __LOCALSTATEDIR, name);
	db = sdbm_open(db_file, O_RDONLY, 0);
	free(db_file);
	
	if (db == NULL) {
		LOGPWARN("sdbm_open");
		return NULL;
	}
	
	k.dptr  = key;
	k.dsize = strlen(k.dptr);
	
	v = sdbm_fetch(db, k);
	
	sdbm_close(db);
	
	if (v.dsize > 0) {
		buf = malloc(v.dsize + 1);
		bzero(buf, v.dsize + 1);
		memcpy(buf, v.dptr, v.dsize);
		return buf;
	}
	
	return NULL;
}

int vxdb_set(char *name, char *key, char *value)
{
	SDBM *db;
	DATUM k, v;
	
	char *db_file;
	
	if (!vxdb_validkey(key))
		return errno = EINVAL, -1;
	
	mkdir(__LOCALSTATEDIR "/vx", 0600);
	
	asprintf(&db_file, "%s/vx/%s", __LOCALSTATEDIR, name);
	db = sdbm_open(db_file, O_RDWR|O_CREAT, 0600);
	free(db_file);
	
	if (db == NULL) {
		LOGPWARN("sdbm_open");
		return -1;
	}
	
	k.dptr  = key;
	k.dsize = strlen(k.dptr);
	
	v.dptr  = value;
	v.dsize = strlen(v.dptr);
	
	if (sdbm_store(db, k, v, SDBM_REPLACE) == -1) {
		LOGPWARN("sdbm_store");
		sdbm_close(db);
		return -1;
	}
	
	sdbm_close(db);
	return 0;
}
