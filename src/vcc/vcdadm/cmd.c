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

#include <stdlib.h>
#include <lucid/printf.h>

#include "vcc.h"
#include "cmd.h"

cmd_t CMDS[] = {
	{ "status",           cmd_status },
	{ "user.set",         cmd_user_set },
	{ "user.get",         cmd_user_get },
	{ "user.remove",      cmd_user_remove },
	{ "user.caps.add",    cmd_user_caps_add },
	{ "user.caps.get",    cmd_user_caps_get },
	{ "user.caps.remove", cmd_user_caps_remove },
	{ NULL,               NULL }
};

void usage(int rc)
{
	printf("Usage: vcdadm <opts>* <command>\n"
			"\n"
			"Available commands:\n"
			"   status\n"
			"\n"
			"   user.set         <username> <admin> [<password>]\n"
			"   user.get         [<username>]\n"
			"   user.remove      <username>\n"
			"\n"
			"   user.caps.add    <username> <cap>\n"
			"   user.caps.get    <username>\n"
			"   user.caps.remove <username> [<cap>]\n"
			"\n"
			COMMON_USAGE);
	exit(rc);
}
