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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <lucid/log.h>
#include <lucid/mem.h>
#include <lucid/strtok.h>

#include "cmd.h"

void cmd_exec(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *result;
	char *command, *output;

	if (argc < 1)
		usage(EXIT_FAILURE);

	strtok_t st;

	if (!strtok_init_argv(&st, argv, argc, 0))
		log_perror_and_die("strtok_init_argv");

	if (strtok_tostr(&st, &command, " ") == -1)
		log_perror_and_die("strtok_tostr");

	result = xmlrpc_client_call(env, uri, "vx.exec",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"command", command);
	return_if_fault(env);

	mem_free(command);

	xmlrpc_decompose_value(env, result, "s", &output);
	return_if_fault(env);

	printf("%s", output);

	xmlrpc_DECREF(result);
}
