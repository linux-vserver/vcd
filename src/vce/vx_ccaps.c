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

#include "cmd.h"

void cmd_vx_ccaps_add(xmlrpc_env *env, int argc, char **argv)
{
	char *ccap;

	if (argc < 1)
		usage(EXIT_FAILURE);

	ccap = argv[0];

	xmlrpc_client_call(env, uri, "vxdb.vx.ccaps.add",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"ccap", ccap);
}

void cmd_vx_ccaps_get(xmlrpc_env *env, int argc, char **argv)
{
	char *ccap;
	xmlrpc_value *response, *result;
	int len, i;

	response = xmlrpc_client_call(env, uri, "vxdb.vx.ccaps.get",
		SIGNATURE("{s:s}"),
		"name", name);
	return_if_fault(env);

	len = xmlrpc_array_size(env, response);
	return_if_fault(env);

	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);

		xmlrpc_decompose_value(env, result, "s", &ccap);
		return_if_fault(env);

		xmlrpc_DECREF(result);

		printf("%s\n", ccap);
	}

	xmlrpc_DECREF(response);
}

void cmd_vx_ccaps_remove(xmlrpc_env *env, int argc, char **argv)
{
	char *ccap;

	if (argc < 1)
		ccap = "";
	else
		ccap = argv[0];

	xmlrpc_client_call(env, uri, "vxdb.vx.ccaps.remove",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"ccap", ccap);
}
