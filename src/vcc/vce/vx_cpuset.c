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
#include <lucid/scanf.h>
#include <lucid/str.h>

#include "vcc.h"
#include "cmd.h"

void cmd_vx_cpuset_get(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *response;
	char *name, *cpus, *mems;
	int virtualize = 0;

	if (argc < 1)
		usage(EXIT_FAILURE);

	name = argv[0];

	response = client_call("vxdb.vx.cpuset.get",
		"{s:s}",
		"name", name);
	return_if_fault(env);

	xmlrpc_decompose_value(env, response,
		"{s:s,s:s,s:i,*}",
		"cpus", &cpus,
		"mems", &mems,
		"virtualize", &virtualize);
	return_if_fault(env);

	xmlrpc_DECREF(response);

	if (!str_isempty(cpus)) {
		printf("cpus=%s\n", cpus);
		printf("mems=%s\n", mems);
		printf("virtualize=%d\n", virtualize);
	}
}

void cmd_vx_cpuset_remove(xmlrpc_env *env, int argc, char **argv)
{
	char *name;

	if (argc < 1)
		usage(EXIT_FAILURE);

	name = argv[0];

	client_call("vxdb.vx.cpuset.remove",
		"{s:s}",
		"name", name);
}

void cmd_vx_cpuset_set(xmlrpc_env *env, int argc, char **argv)
{
	char *name, *cpus, *mems;
	int virtualize = 0;

	if (argc < 3)
		usage(EXIT_FAILURE);

	name = argv[0];
	cpus = argv[1];
	mems = argv[2];

	if (argc > 3)
		sscanf(argv[3], "%d", &virtualize);

	client_call("vxdb.vx.cpuset.set",
		"{s:s,s:s,s:s,s:b}",
		"name", name,
		"cpus", cpus,
		"mems", mems,
		"virtualize", virtualize);
}
