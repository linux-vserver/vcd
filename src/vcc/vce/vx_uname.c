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

void cmd_vx_uname_get(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *response, *result;
	char *name, *type, *value;
	int len, i;

	if (argc < 1)
		usage(EXIT_FAILURE);

	name = argv[0];

	if (argc > 1)
		type = argv[1];
	else
		type = "";

	response = client_call("vxdb.vx.uname.get",
		"{s:s,s:s}",
		"name", name,
		"type", type);
	return_if_fault(env);

	len = xmlrpc_array_size(env, response);
	return_if_fault(env);

	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);

		xmlrpc_decompose_value(env, result,
			"{s:s,s:s,*}",
			"type", &type,
			"value", &value);
		return_if_fault(env);

		xmlrpc_DECREF(result);

		printf("%s=%s\n", type, value);
	}

	xmlrpc_DECREF(response);
}

void cmd_vx_uname_remove(xmlrpc_env *env, int argc, char **argv)
{
	char *name, *type;

	if (argc < 1)
		usage(EXIT_FAILURE);

	name = argv[0];

	if (argc > 1)
		type = argv[1];
	else
		type = "";

	client_call("vxdb.vx.uname.remove",
		"{s:s,s:s}",
		"name", name,
		"type", type);
}

void cmd_vx_uname_set(xmlrpc_env *env, int argc, char **argv)
{
	char *name, *type, *value;

	if (argc < 3)
		usage(EXIT_FAILURE);

	name = argv[0];
	type = argv[1];
	value = argv[2];

	client_call("vxdb.vx.uname.set",
		"{s:s,s:s,s:s}",
		"name", name,
		"type", type,
		"value", value);
}
