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

#include "cmd.h"

void cmd_dx_limit_get(xmlrpc_env *env, int argc, char **argv)
{
	char *path;
	int space, inodes, reserved;
	xmlrpc_value *response, *result;
	int len, i;
	
	if (argc < 1)
		path = "";
	else
		path = argv[0];
	
	response = xmlrpc_client_call(env, uri, "vxdb.dx.limit.get",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"path", path);
	return_if_fault(env);
	
	len = xmlrpc_array_size(env, response);
	return_if_fault(env);
	
	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);
		
		xmlrpc_decompose_value(env, result,
			"{s:s,s:i,s:i,s:i,*}",
			"path", &path,
			"space", &space,
			"inodes", &inodes,
			"reserved", &reserved);
		return_if_fault(env);
		
		xmlrpc_DECREF(result);
		
		printf("%s: %u %u %u\n", path, space, inodes, reserved);
	}
	
	xmlrpc_DECREF(response);
}

void cmd_dx_limit_remove(xmlrpc_env *env, int argc, char **argv)
{
	char *path;
	
	if (argc < 1)
		path = "";
	else
		path = argv[0];
	
	xmlrpc_client_call(env, uri, "vxdb.dx.limit.remove",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"path", path);
}

void cmd_dx_limit_set(xmlrpc_env *env, int argc, char **argv)
{
	char *path;
	int space, inodes, reserved;
	
	if (argc < 4)
		usage(EXIT_FAILURE);
	
	path = argv[0];
	space = atoi(argv[1]);
	inodes = atoi(argv[2]);
	reserved = atoi(argv[3]);
	
	xmlrpc_client_call(env, uri, "vxdb.dx.limit.set",
		SIGNATURE("{s:s,s:s,s:i,s:i,s:i}"),
		"name", name,
		"path", path,
		"space", space,
		"inodes", inodes,
		"reserved", reserved);
}
