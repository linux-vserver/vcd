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

#include "cmd.h"

char *uri  = NULL;
char *user = "admin";
char *pass = NULL;
char *name = NULL;

cmd_t CMDS[] = {
	{ "create",  cmd_create },
	{ "exec",    cmd_exec },
	{ "kill",    cmd_kill },
	{ "login",   cmd_login },
	{ "reboot",  cmd_reboot },
	{ "remove",  cmd_remove },
	{ "rename",  cmd_rename },
	{ "start",   cmd_start },
	{ "status",  cmd_status },
	{ "stop",    cmd_stop },
	{ NULL,      NULL }
};
