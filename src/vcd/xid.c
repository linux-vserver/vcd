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
	xid_t foundxid;
	
	if (!(db = sdbm_open(__LOCALSTATEDIR "/maps/xid", O_RDONLY, 0)))
		return log_warn("sdbm_open: %s", strerror(errno)), -1;
	
	k.dptr  = name;
	k.dsize = strlen(k.dptr);
	
	v = sdbm_fetch(db, k);
	
	sdbm_close(db);
	
	if (v.dsize > 0) {
		buf = strndup(v.dptr, v.dsize);
		foundxid = atoi(buf);
		free(buf);
	}
	
	else
		return errno = ESRCH, -1;
	
	*xid = foundxid;
	return 0;
}

int xid_toname(xid_t xid, char **name)
{
	SDBM *db;
	DATUM k, v;
	
	char *buf;
	xid_t foundxid;
	
	*name = NULL;
	
	if (!(db = sdbm_open(__LOCALSTATEDIR "/maps/xid", O_RDONLY, 0)))
		return log_warn("sdbm_open: %s", strerror(errno)), -1;
	
	for (k = sdbm_firstkey(db); k.dsize > 0; k = sdbm_nextkey(db)) {
		v = sdbm_fetch(db, k);
		
		if (v.dsize > 0) {
			buf = strndup(v.dptr, v.dsize);
			foundxid = atoi(buf);
			free(buf);
			
			if (foundxid == xid) {
				buf = strndup(k.dptr, k.dsize);
				*name = buf;
				break;
			}
		}
	}
	
	sdbm_close(db);
	
	if (*name)
		return 0;
	
	return -1;
}

int xid_isvalid(char *name, xid_t xid)
{
	struct vx_vhi_name vhiname;
	
	bzero(&vhiname, sizeof(vhiname));
	strncpy(vhiname.name, name, VHILEN);
	
	if (vx_get_info(xid, NULL) == -1) {
		if (errno == ESRCH)
			return 1;
		else
			return 0;
	}
	
	if (vx_get_vhi_name(xid, &vhiname) == -1)
		return 0;
	
	if (strcmp(name, vhiname.name) == 0)
		return 1;
	
	return 0;
}
