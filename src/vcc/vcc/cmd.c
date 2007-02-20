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
	{ "create",    cmd_create },
	{ "exec",      cmd_exec },
	{ "kill",      cmd_kill },
	{ "list",      cmd_list },
	{ "load",      cmd_load },
	{ "login",     cmd_login },
	{ "reboot",    cmd_reboot },
	{ "remove",    cmd_remove },
	{ "rename",    cmd_rename },
	{ "restart",   cmd_restart },
	{ "start",     cmd_start },
	{ "status",    cmd_status },
	{ "stop",      cmd_stop },
	{ "templates", cmd_templates },
	{ NULL,        NULL }
};

void usage(int rc)
{
	printf("Usage: vcc <opts>* <command>\n"
			"\n"
			"Available commands:\n"
			"   create    <name> <template> [<force> [<copy> [<vdir>]]]\n"
			"   exec      <name> <command> <args>*\n"
			"   kill      <name> <pid> <sig>\n"
			"   list      [<username>]\n"
			"   load      <name>\n"
			"   login     <name>\n"
			"   reboot    <name>\n"
			"   remove    <name>\n"
			"   rename    <name> <newname>\n"
			"   start     <name>\n"
			"   status    <name>\n"
			"   stop      <name>\n"
			"   templates [<name>]\n"
			"\n"
			COMMON_USAGE);
	exit(rc);
}
