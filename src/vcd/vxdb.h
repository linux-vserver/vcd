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

#ifndef _VCD_VXDB_H
#define _VCD_VXDB_H

#include <sqlite3.h>
#include <vserver.h>

/* sqlite3 api returns unsigned char, we always want signed char though */
#define sqlite3_column_text  (const char *) sqlite3_column_text

typedef sqlite3_stmt vxdb_result;

extern sqlite3 *vxdb;

void vxdb_init(void);
void vxdb_atexit(void);

int vxdb_prepare(vxdb_result **dbr, const char *fmt, ...);
int vxdb_step(vxdb_result *dbr);

#define vxdb_foreach_step(RC, DBR) \
	for (RC = vxdb_step(DBR); RC == 1; RC = vxdb_step(DBR))

int vxdb_exec(const char *fmt, ...);

xid_t vxdb_getxid(const char *name);
char *vxdb_getname(xid_t xid);

#endif
