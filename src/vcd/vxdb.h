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

typedef sqlite3_stmt vxdb_result;

extern sqlite3 *vxdb;
extern vxdb_result *dbr;

/* sqlite api aliases */
#define vxdb_errmsg   sqlite3_errmsg
#define vxdb_changes  sqlite3_changes

/* sqlite return code aliases */
#define VXDB_OK   SQLITE_OK
#define VXDB_DONE SQLITE_DONE
#define VXDB_ROW  SQLITE_ROW
#define VXDB_BUSY SQLITE_BUSY

void vxdb_init(void);
void vxdb_atexit(void);

int vxdb_prepare(vxdb_result **dbr, const char *fmt, ...);
int vxdb_step(vxdb_result *dbr);
int vxdb_finalize(vxdb_result *dbr);

#define vxdb_foreach_step(RC, DBR) \
	for (RC = vxdb_step(DBR); RC == VXDB_ROW; RC = vxdb_step(DBR))

int vxdb_exec(const char *fmt, ...);

const char *vxdb_column_text(vxdb_result *dbr, int col);

int      vxdb_column_int(vxdb_result *dbr, int col);
int32_t  vxdb_column_int32(vxdb_result *dbr, int col);
uint32_t vxdb_column_uint32(vxdb_result *dbr, int col);
int64_t  vxdb_column_int64(vxdb_result *dbr, int col);
uint64_t vxdb_column_uint64(vxdb_result *dbr, int col);

xid_t vxdb_getxid(const char *name);
char *vxdb_getname(xid_t xid);
char *vxdb_getvdir(const char *name);
int   vxdb_getgid(const char *groupname);

#endif
