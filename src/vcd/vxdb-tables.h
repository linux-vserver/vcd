// Copyright 2007 Benedikt Böhm <hollow@gentoo.org>
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

#ifndef _VCD_VXDB_TABLES_H
#define _VCD_VXDB_TABLES_H

struct _vxdb_table {
	const char *name;
	const char **columns;
	const char **unique;
};

#define VXDB_VERSION_MAJOR 1
#define VXDB_VERSION_MINOR 1
#define VXDB_VERSION ((VXDB_VERSION_MAJOR << 8) | VXDB_VERSION_MINOR)

#define VXDB_COLUMNS_NEW(NAME, ...) \
	static const char *_ ## NAME ## _columns[] = { __VA_ARGS__, NULL };

#define VXDB_UNIQUE_NEW(NAME, ...) \
	static const char *_ ## NAME ## _unique[] = { __VA_ARGS__, NULL };

#define VXDB_UNIQUE_NONE(NAME) \
	static const char *_ ## NAME ## _unique[] = { NULL };

#define VXDB_DATABASE_START \
	static struct _vxdb_table _vxdb_tables[] = {

#define VXDB_TABLE_NEW(NAME) \
		{ #NAME, _ ## NAME ## _columns, _ ## NAME ## _unique },

#define VXDB_DATABASE_END \
		{ NULL, NULL, NULL } \
	};

/* generate table definition */
VXDB_COLUMNS_NEW(dx_limit, "xid", "space", "inodes", "reserved")
VXDB_UNIQUE_NEW(dx_limit, "xid")

VXDB_COLUMNS_NEW(groups, "gid", "name")
VXDB_UNIQUE_NEW(groups, "gid", "name")

VXDB_COLUMNS_NEW(init, "xid", "init", "halt", "reboot", "timeout")
VXDB_UNIQUE_NEW(init, "xid")

VXDB_COLUMNS_NEW(mount, "xid", "src", "dst", "type", "opts")
VXDB_UNIQUE_NEW(mount, "xid,dst")

VXDB_COLUMNS_NEW(nx_addr, "xid", "addr", "netmask")
VXDB_UNIQUE_NEW(nx_addr, "xid,addr")

VXDB_COLUMNS_NEW(nx_broadcast, "xid", "broadcast")
VXDB_UNIQUE_NEW(nx_broadcast, "xid")

VXDB_COLUMNS_NEW(restart, "xid")
VXDB_UNIQUE_NEW(restart, "xid")

VXDB_COLUMNS_NEW(user, "uid", "name", "password", "admin")
VXDB_UNIQUE_NEW(user, "uid", "name")

VXDB_COLUMNS_NEW(user_caps, "uid", "cap")
VXDB_UNIQUE_NEW(user_caps, "uid,cap")

VXDB_COLUMNS_NEW(vcd, "uptime", "requests", "flogins",
		"nomethod", "vxdbqueries")
VXDB_UNIQUE_NEW(vcd, "uptime")

VXDB_COLUMNS_NEW(vdir, "xid", "vdir")
VXDB_UNIQUE_NEW(vdir, "xid", "vdir")

VXDB_COLUMNS_NEW(vx_bcaps, "xid", "bcap")
VXDB_UNIQUE_NEW(vx_bcaps, "xid,bcap")

VXDB_COLUMNS_NEW(vx_ccaps, "xid", "ccap")
VXDB_UNIQUE_NEW(vx_ccaps, "xid,ccap")

VXDB_COLUMNS_NEW(vx_flags, "xid", "flag")
VXDB_UNIQUE_NEW(vx_flags, "xid,flag")

VXDB_COLUMNS_NEW(vx_limit, "xid", "type", "soft", "max")
VXDB_UNIQUE_NEW(vx_limit, "xid,type")

VXDB_COLUMNS_NEW(vx_sched, "xid", "cpuid",
		"fillrate", "fillrate2",
		"interval", "interval2",
		"tokensmin", "tokensmax")
VXDB_UNIQUE_NEW(vx_sched, "xid,cpuid")

VXDB_COLUMNS_NEW(vx_uname, "xid", "type", "value")
VXDB_UNIQUE_NEW(vx_uname, "xid,type")

VXDB_COLUMNS_NEW(xid_gid_map, "xid", "gid")
VXDB_UNIQUE_NONE(xid_gid_map)

VXDB_COLUMNS_NEW(xid_name_map, "xid", "name")
VXDB_UNIQUE_NEW(xid_name_map, "xid", "name")

VXDB_COLUMNS_NEW(xid_uid_map, "xid", "uid")
VXDB_UNIQUE_NONE(xid_uid_map)

/* generate database definition */
VXDB_DATABASE_START
VXDB_TABLE_NEW(dx_limit)
VXDB_TABLE_NEW(groups)
VXDB_TABLE_NEW(init)
VXDB_TABLE_NEW(mount)
VXDB_TABLE_NEW(nx_addr)
VXDB_TABLE_NEW(nx_broadcast)
VXDB_TABLE_NEW(restart)
VXDB_TABLE_NEW(user)
VXDB_TABLE_NEW(user_caps)
VXDB_TABLE_NEW(vcd)
VXDB_TABLE_NEW(vdir)
VXDB_TABLE_NEW(vx_bcaps)
VXDB_TABLE_NEW(vx_ccaps)
VXDB_TABLE_NEW(vx_flags)
VXDB_TABLE_NEW(vx_limit)
VXDB_TABLE_NEW(vx_sched)
VXDB_TABLE_NEW(vx_uname)
VXDB_TABLE_NEW(xid_gid_map)
VXDB_TABLE_NEW(xid_name_map)
VXDB_TABLE_NEW(xid_uid_map)
VXDB_DATABASE_END

#endif
