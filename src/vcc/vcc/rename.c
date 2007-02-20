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

#include "vcc.h"
#include "cmd.h"

void cmd_rename(xmlrpc_env *env, int argc, char **argv)
{
	char *name, *newname;

	if (argc < 2)
		usage(EXIT_FAILURE);

	name    = argv[0];
	newname = argv[1];

	client_call("vx.rename", "{s:s,s:s}", "name", name, "newname", newname);
}
