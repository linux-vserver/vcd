// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
//           2007 Luca Longinotti <chtekk@gentoo.org>
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

void cmd_limstatus(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *response, *result;
	char *name, *type;
	int min = 0, cur = 0, max = 0;
	int len, i;

	if (argc < 1)
		usage(EXIT_FAILURE);

	name = argv[0];

	response = client_call("vx.limstatus",
		"{s:s}",
		"name", name);
	return_if_fault(env);

	len = xmlrpc_array_size(env, response);
	return_if_fault(env);

	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);

		xmlrpc_decompose_value(env, result,
			"{s:s,s:i,s:i,s:i,*}",
			"type", &type,
			"min",  &min,
			"cur",  &cur,
			"max",  &max);
		return_if_fault(env);

		xmlrpc_DECREF(result);

		printf("%-12s - cur: %d, min: %d, max: %d\n", type, cur, min, max);
	}

	xmlrpc_DECREF(response);
}
