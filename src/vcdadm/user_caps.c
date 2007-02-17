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

#include "cmd.h"

void cmd_user_caps_add(xmlrpc_env *env, int argc, char **argv)
{
	char *cap;

	if (argc < 1)
		usage(EXIT_FAILURE);

	cap = argv[0];

	xmlrpc_client_call(env, uri, "vcd.user.caps.add",
		SIGNATURE("{s:s,s:s}"),
		"username", name,
		"cap",      cap);
}

void cmd_user_caps_get(xmlrpc_env *env, int argc, char **argv)
{
	char *cap;
	xmlrpc_value *response, *result;
	int len, i;

	response = xmlrpc_client_call(env, uri, "vcd.user.caps.get",
		SIGNATURE("{s:s}"),
		"username", name);
	return_if_fault(env);

	len = xmlrpc_array_size(env, response);
	return_if_fault(env);

	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);

		xmlrpc_decompose_value(env, result, "s", &cap);
		return_if_fault(env);

		xmlrpc_DECREF(result);

		printf("%s\n", cap);
	}

	xmlrpc_DECREF(response);
}

void cmd_user_caps_remove(xmlrpc_env *env, int argc, char **argv)
{
	char *cap;

	if (argc < 1)
		cap = "";
	else
		cap = argv[0];

	xmlrpc_client_call(env, uri, "vcd.user.caps.remove",
		SIGNATURE("{s:s,s:s}"),
		"username", name,
		"cap",  cap);
}
