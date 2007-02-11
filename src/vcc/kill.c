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

#include <lucid/scanf.h>

#include "cmd.h"

void cmd_kill(xmlrpc_env *env, int argc, char **argv)
{
	int pid = 0, sig = 0;

	if (argc < 2)
		usage(EXIT_FAILURE);

	sscanf(argv[0], "%d", &pid);
	sscanf(argv[1], "%d", &sig);

	xmlrpc_client_call(env, uri, "vx.kill",
		SIGNATURE("{s:s,s:i,s:i}"),
		"name", name,
		"pid", pid,
		"sig", sig);
}
