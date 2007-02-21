// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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

#include <stdlib.h>
#include <lucid/printf.h>

#include "vcc.h"
#include "cmd.h"

cmd_t CMDS[] = {
	{ "dep.add",             cmd_dep_add },
	{ "dep.remove",          cmd_dep_remove },
	{ "dx.limit.get",        cmd_dx_limit_get },
	{ "dx.limit.remove",     cmd_dx_limit_remove },
	{ "dx.limit.set",        cmd_dx_limit_set },
	{ "init.get",            cmd_init_get },
	{ "init.set",            cmd_init_set },
	{ "list",                cmd_list },
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
	{ NULL,                  NULL }
};

void usage(int rc)
{
	printf("Usage: vce <opts>* <command>\n"
			"\n"
			"Available commands:\n"
			"   list                [<username>]\n"
			"\n"
	        "   dep.add             <name> <depname>\n"
	        "   dep.remove          <name> <depname>\n"
	        "\n"
			"   dx.limit.get        <name>\n"
			"   dx.limit.remove     <name>\n"
			"   dx.limit.set        <name> <space> <inodes> <reserved>\n"
			"\n"
			"   init.get            <name>\n"
			"   init.set            <name> <init>\n"
			"\n"
			"   <init> = [<init> [<halt> [<reboot> [<timeout>]]]]\n"
			"\n"
			"   mount.get           <name> [<dst>]\n"
			"   mount.remove        <name> [<dst>]\n"
			"   mount.set           <name> <src> <dst> [<type> [<opts>]]\n"
			"\n"
			"   nx.broadcast.get    <name>\n"
			"   nx.broadcast.remove <name>\n"
			"   nx.broadcast.set    <name> <broadcast>\n"
			"\n"
			"   nx.addr.get         <name> [<addr>]\n"
			"   nx.addr.remove      <name> [<addr>]\n"
			"   nx.addr.set         <name> <addr> [<netmask>]\n"
			"\n"
			"   vx.bcaps.add        <name> <bcap>\n"
			"   vx.bcaps.get        <name>\n"
			"   vx.bcaps.remove     <name> [<bcap>]\n"
			"\n"
			"   vx.ccaps.add        <name> <ccap>\n"
			"   vx.ccaps.get        <name>\n"
			"   vx.ccaps.remove     <name> [<ccap>]\n"
			"\n"
			"   vx.flags.add        <name> <flag>\n"
			"   vx.flags.get        <name>\n"
			"   vx.flags.remove     <name> [<flag>]\n"
			"\n"
			"   vx.limit.get        <name> [<limit>]\n"
			"   vx.limit.remove     <name> [<limit>]\n"
			"   vx.limit.set        <name> <limit> <soft> [<max>]\n"
			"\n"
			"   vx.sched.get        <name> [<cpuid>]\n"
			"   vx.sched.remove     <name> [<cpuid>]\n"
			"   vx.sched.set        <name> <cpuid> <bucket>\n"
			"\n"
			"   <bucket> = <int> <fill> <int2> <fill2> <min> <max>\n"
			"\n"
			"   vx.uname.get        <name> [<uname>]\n"
			"   vx.uname.remove     <name> [<uname>]\n"
			"   vx.uname.set        <name> <uname> <value>\n"
			"\n"
			COMMON_USAGE);
	exit(rc);
}
