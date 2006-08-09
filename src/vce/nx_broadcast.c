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

#include <stdlib.h>
#include <stdio.h>
#include <lucid/str.h>

#include "cmd.h"

void cmd_nx_broadcast_get(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *response;
	char *broadcast;
	
	response = xmlrpc_client_call(env, uri, "vxdb.nx.broadcast.get",
		SIGNATURE("{s:s}"),
		"name", name);
	return_if_fault(env);
	
	xmlrpc_decompose_value(env, response, "s", &broadcast);
	xmlrpc_DECREF(response);
	
	if (!str_isempty(broadcast))
		printf("%s\n", broadcast);
}

void cmd_nx_broadcast_remove(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_client_call(env, uri, "vxdb.nx.broadcast.remove",
		SIGNATURE("{s:s}"),
		"name", name);
}

void cmd_nx_broadcast_set(xmlrpc_env *env, int argc, char **argv)
{
	char *broadcast;
	
	if (argc < 1)
		usage(EXIT_FAILURE);
	
	broadcast = argv[0];
	
	xmlrpc_client_call(env, uri, "vxdb.nx.broadcast.set",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"broadcast", broadcast);
}
