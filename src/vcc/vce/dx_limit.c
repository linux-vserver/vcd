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
#include <lucid/scanf.h>
#include <lucid/str.h>

#include "vcc.h"
#include "cmd.h"

void cmd_dx_limit_get(xmlrpc_env *env, int argc, char **argv)
{
	char *space, *inodes;
	int reserved;
	xmlrpc_value *response;

	response = client_call("vxdb.dx.limit.get",
		"{s:s}",
		"name", name);
	return_if_fault(env);

	xmlrpc_decompose_value(env, response,
        "{s:s,s:s,s:i,*}",
		"space", &space,
		"inodes", &inodes,
		"reserved", &reserved);
	return_if_fault(env);

	xmlrpc_DECREF(response);

	if (!str_isempty(space))
		printf("disk limits: %s %s %d\n", space, inodes, reserved);
}

void cmd_dx_limit_remove(xmlrpc_env *env, int argc, char **argv)
{
	client_call("vxdb.dx.limit.remove",
		"{s:s}",
		"name", name);
}

void cmd_dx_limit_set(xmlrpc_env *env, int argc, char **argv)
{
	char *space, *inodes;
	int reserved;

	if (argc < 3)
		usage(EXIT_FAILURE);

	space = argv[0];
	inodes = argv[1];
	sscanf(argv[2], "%d", &reserved);

	client_call("vxdb.dx.limit.set",
		"{s:s,s:s,s:s,s:i}",
		"name", name,
		"space", space,
		"inodes", inodes,
		"reserved", reserved);
}
