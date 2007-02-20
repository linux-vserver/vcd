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
#include <lucid/str.h>

#include "vcc.h"
#include "cmd.h"

void cmd_nx_broadcast_get(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *response;
	char *broadcast;

	if (argc < 1)
		usage(EXIT_FAILURE);

	char *name = argv[0];

	response = client_call("vxdb.nx.broadcast.get", "{s:s}", "name", name);
	return_if_fault(env);

	xmlrpc_decompose_value(env, response, "s", &broadcast);
	xmlrpc_DECREF(response);

	if (!str_isempty(broadcast))
		printf("%s\n", broadcast);
}

void cmd_nx_broadcast_remove(xmlrpc_env *env, int argc, char **argv)
{
	if (argc < 1)
		usage(EXIT_FAILURE);

	char *name = argv[0];

	client_call("vxdb.nx.broadcast.remove", "{s:s}", "name", name);
}

void cmd_nx_broadcast_set(xmlrpc_env *env, int argc, char **argv)
{
	char *name, *broadcast;

	if (argc < 2)
		usage(EXIT_FAILURE);

	name = argv[0];
	broadcast = argv[1];

	client_call("vxdb.nx.broadcast.set", "{s:s,s:s}",
		"name", name,
		"broadcast", broadcast);
}
