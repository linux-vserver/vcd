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
	{ "add",       cmd_groupadd },
	{ "list",      cmd_grouplist },
	{ "remove",    cmd_groupremove },
	{ "vx.add",    cmd_vserveradd },
	{ "vx.list",   cmd_vserverlist },
	{ "vx.remove", cmd_vserverremove },
	{ "vx.start",  cmd_wrapperstart },
	{ "vx.stop",   cmd_wrapperstop },
	{ NULL,        NULL }
};

void usage(int rc)
{
	printf("Usage: vgc <opts>* <command>\n"
			"\n"
			"Available commands:\n"
			"   list\n"
			"   add       <group>\n"
			"   remove    <group>\n"
			"\n"
			"   vx.list   <group>\n"
			"   vx.add    <group> <vserver>\n"
			"   vx.remove <group> <vserver>\n"
			"\n"
			"   vx.start  <group>\n"
			"   vx.stop   <group>\n"
			"\n"
			COMMON_USAGE);
	exit(rc);
}

