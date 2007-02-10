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

void cmd_vx_limit_get(xmlrpc_env *env, int argc, char **argv)
{
	char *type;
	int soft, max;
	xmlrpc_value *response, *result;
	int len, i;
	
	if (argc < 1)
		type = "";
	else
		type = argv[0];
	
	response = xmlrpc_client_call(env, uri, "vxdb.vx.limit.get",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"type", type);
	return_if_fault(env);
	
	len = xmlrpc_array_size(env, response);
	return_if_fault(env);
	
	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);
		
		xmlrpc_decompose_value(env, result,
			"{s:s,s:i,s:i,*}",
			"type", &type,
			"soft", &soft,
			"max", &max);
		return_if_fault(env);
		
		xmlrpc_DECREF(result);
		
		printf("%s: %d %d\n", type, soft, max);
	}
	
	xmlrpc_DECREF(response);
}

void cmd_vx_limit_remove(xmlrpc_env *env, int argc, char **argv)
{
	char *type;
	
	if (argc < 1)
		type = "";
	else
		type = argv[0];
	
	xmlrpc_client_call(env, uri, "vxdb.vx.limit.remove",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"type", type);
}

void cmd_vx_limit_set(xmlrpc_env *env, int argc, char **argv)
{
	char *type;
	int soft, max;
	
	if (argc < 2)
		usage(EXIT_FAILURE);
	
	type = argv[0];
	sscanf(argv[1], "%d", &soft);
	
	if (argc > 2)
		sscanf(argv[2], "%d", &max);
	else
		max = soft;
	
	xmlrpc_client_call(env, uri, "vxdb.vx.limit.set",
		SIGNATURE("{s:s,s:s,s:i,s:i}"),
		"name", name,
		"type", type,
		"soft", soft,
		"max", max);
}
