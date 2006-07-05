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

#include "cmd.h"

void cmd_rename(xmlrpc_env *env, int argc, char **argv)
{
	char *newname;
	
	if (argc != 1)
		usage(EXIT_FAILURE);
	
	newname = argv[0];
	
	xmlrpc_client_call(env, uri, "vx.rename",
		SIGNATURE("{s:s,s:s}"),
		"name", name,
		"newname", newname);
}
