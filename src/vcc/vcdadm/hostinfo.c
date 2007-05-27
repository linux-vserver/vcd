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

#include <lucid/printf.h>

#include "vcc.h"
#include "cmd.h"

void cmd_hostinfo(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *result;
	char *vbasedir;
	int cpus = 0, memnodes = 0;

	result = client_call0("vcd.hostinfo");
	return_if_fault(env);

	xmlrpc_decompose_value(env, result,
		"{s:s,s:i,s:i,*}",
		"vbasedir", &vbasedir,
		"cpus", &cpus,
		"memnodes", &memnodes);
	return_if_fault(env);

	xmlrpc_DECREF(result);

	printf("vbasedir: %s\n", vbasedir);
	printf("cpus: %d\n", cpus);
	printf("memnodes: %d\n", memnodes);
}
