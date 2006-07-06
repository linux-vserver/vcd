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

#include "cmd.h"

char *uri  = NULL;
char *user = "admin";
char *pass = NULL;
char *name = NULL;

cmd_t CMDS[] = {
	{ "dx.limit.get",        cmd_dx_limit_get },
	{ "dx.limit.remove",     cmd_dx_limit_remove },
	{ "dx.limit.set",        cmd_dx_limit_set },
	{ "init.method.get",     cmd_init_method_get },
	{ "init.method.set",     cmd_init_method_set },
	{ "mount.get",           cmd_mount_get },
	{ "mount.remove",        cmd_mount_remove },
	{ "mount.set",           cmd_mount_set },
	{ "nx.addr.get",         cmd_nx_addr_get },
	{ "nx.addr.remove",      cmd_nx_addr_remove },
	{ "nx.addr.set",         cmd_nx_addr_set },
	{ "nx.broadcast.get",    cmd_nx_broadcast_get },
	{ "nx.broadcast.remove", cmd_nx_broadcast_remove },
	{ "nx.broadcast.set",    cmd_nx_broadcast_set },
	{ "vx.bcaps.add",        cmd_vx_bcaps_add },
	{ "vx.bcaps.get",        cmd_vx_bcaps_get },
	{ "vx.bcaps.remove",     cmd_vx_bcaps_remove },
	{ "vx.ccaps.add",        cmd_vx_ccaps_add },
	{ "vx.ccaps.get",        cmd_vx_ccaps_get },
	{ "vx.ccaps.remove",     cmd_vx_ccaps_remove },
	{ "vx.flags.add",        cmd_vx_flags_add },
	{ "vx.flags.get",        cmd_vx_flags_get },
	{ "vx.flags.remove",     cmd_vx_flags_remove },
	{ "vx.limit.get",        cmd_vx_limit_get },
	{ "vx.limit.remove",     cmd_vx_limit_remove },
	{ "vx.limit.set",        cmd_vx_limit_set },
	{ "vx.sched.get",        cmd_vx_sched_get },
	{ "vx.sched.remove",     cmd_vx_sched_remove },
	{ "vx.sched.set",        cmd_vx_sched_set },
	{ "vx.uname.get",        cmd_vx_uname_get },
	{ "vx.uname.remove",     cmd_vx_uname_remove },
	{ "vx.uname.set",        cmd_vx_uname_set },
	{ NULL,      NULL }
};
