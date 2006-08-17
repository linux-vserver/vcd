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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <vserver.h>
#include <lucid/argv.h>

#include "cmd.h"
#include "msg.h"

void cmd_exec(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *result;
	char *command, *output;
	
	if (argc < 1)
		usage(EXIT_FAILURE);
	
	command = argv_to_str(argc, (const char ** const)argv);
	
	result = xmlrpc_client_call(env, uri, "vx.exec",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"command", command);
	return_if_fault(env);
	
	free(command);
	
	xmlrpc_decompose_value(env, result, "s", &output);
	return_if_fault(env);
	
	printf("%s", output);
	
	xmlrpc_DECREF(result);
}
