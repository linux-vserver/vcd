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

#include "cmd.h"

void cmd_mount_get(xmlrpc_env *env, int argc, char **argv)
{
	char *src, *dst, *type, *opts;
	xmlrpc_value *response, *result;
	int len, i;

	if (argc < 1)
		dst = "";
	else
		dst = argv[0];

	response = xmlrpc_client_call(env, uri, "vxdb.mount.get",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"dst", dst);
	return_if_fault(env);

	len = xmlrpc_array_size(env, response);
	return_if_fault(env);

	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);
		
		xmlrpc_decompose_value(env, result,
			"{s:s,s:s,s:s,s:s,*}",
			"src", &src,
			"dst", &dst,
			"type", &type,
			"opts", &opts);
		return_if_fault(env);
		
		xmlrpc_DECREF(result);
		
		printf("%s %s %s %s\n", src, dst, type, opts);
	}

	xmlrpc_DECREF(response);
}

void cmd_mount_remove(xmlrpc_env *env, int argc, char **argv)
{
	char *dst;

	if (argc < 1)
		dst = "";
	else
		dst = argv[0];

	xmlrpc_client_call(env, uri, "vxdb.mount.remove",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"dst", dst);
}

void cmd_mount_set(xmlrpc_env *env, int argc, char **argv)
{
	char *src, *dst, *type, *opts;

	if (argc < 2)
		usage(EXIT_FAILURE);

	src  = argv[0];
	dst  = argv[1];

	if (argc > 2)
		type = argv[2];
	else
		type = "";

	if (argc > 3)
		opts = argv[3];
	else
		opts = "";

	xmlrpc_client_call(env, uri, "vxdb.mount.set",
		SIGNATURE("{s:s,s:s,s:s,s:s,s:s}"),
		"name", name,
		"src", src,
		"dst", dst,
		"type", type,
		"opts", opts);
}
