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

#include <lucid/printf.h>
#include <lucid/scanf.h>

#include "cmd.h"

void cmd_init_get(xmlrpc_env *env, int argc, char **argv)
{
	char *init, *halt, *reboot;
	xmlrpc_value *response;
	
	response = xmlrpc_client_call(env, uri, "vxdb.init.get",
		SIGNATURE("{s:s}"),
		"name", name);
	return_if_fault(env);
	
	xmlrpc_decompose_value(env, response,
		"{s:s,s:s,s:s,*}",
		"init", &init,
		"halt", &halt,
		"reboot", &reboot);
	return_if_fault(env);
	
	xmlrpc_DECREF(response);
	
	printf("init=%s\n", init);
	printf("halt=%s\n", halt);
	printf("reboot=%s\n", reboot);
}

void cmd_init_set(xmlrpc_env *env, int argc, char **argv)
{
	char *init, *halt, *reboot;
	int timeout;
	
	if (argc < 4)
		usage(EXIT_FAILURE);
	
	init   = argv[0];
	halt   = argv[1];
	reboot = argv[2];

	sscanf(argv[3], "%d", &timeout);
	
	xmlrpc_client_call(env, uri, "vxdb.init.set",
		SIGNATURE("{s:s,s:s,s:s,s:s,s:i}"),
		"name", name,
		"init", init,
		"halt", halt,
		"reboot", reboot,
		"timeout", timeout);
}
