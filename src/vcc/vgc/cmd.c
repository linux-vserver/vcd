// Copyright 2007 Luca Longinotti <chtekk@gentoo.org>
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
	{ "add",        cmd_add },
	{ "list",       cmd_list },
	{ "remove",     cmd_remove },
	{ "vx.reboot",  cmd_vx_reboot },
	{ "vx.restart", cmd_vx_restart },
	{ "vx.start",   cmd_vx_start },
	{ "vx.stop",    cmd_vx_stop },
	{ NULL,         NULL }
};

void usage(int rc)
{
	printf("Usage: vgc <opts>* <command>\n"
			"\n"
			"Available commands:\n"
			"   add        <group> [<name>]\n"
			"   list       [<group>]\n"
			"   remove     <group> [<name>]\n"
			"\n"
			"   vx.reboot  <group>\n"
			"   vx.restart <group>\n"
			"   vx.start   <group>\n"
			"   vx.stop    <group>\n"
			"\n"
			COMMON_USAGE);
	exit(rc);
}

