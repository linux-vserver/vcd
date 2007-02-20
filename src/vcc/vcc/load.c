// Copyright 2007 Luca Longinotti <chtekk@gentoo.org>
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

void cmd_load(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *result;
	char *name, *loadavg1m, *loadavg5m, *loadavg15m;

	if (argc < 1)
		usage(EXIT_FAILURE);

	name = argv[0];

	result = client_call("vx.load", "{s:s}", "name", name);
	return_if_fault(env);

	xmlrpc_decompose_value(env, result,
		"{s:s,s:s,s:s,*}",
		"1m", &loadavg1m,
		"5m", &loadavg5m,
		"15m", &loadavg15m);
	return_if_fault(env);

	xmlrpc_DECREF(result);

	printf("load average: %s, %s, %s\n", loadavg1m, loadavg5m, loadavg15m);
}
