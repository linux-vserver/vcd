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

#include "vcc.h"
#include "cmd.h"

void cmd_vx_start(xmlrpc_env *env, int argc, char **argv)
{
	if (argc < 1)
		usage(EXIT_FAILURE);

	char *group = argv[0];

	client_call("vg.vx.start", "{s:s}", "group", group);
}

void cmd_vx_stop(xmlrpc_env *env, int argc, char **argv)
{
	if (argc < 1)
		usage(EXIT_FAILURE);

	char *group = argv[0];

	client_call("vg.vx.stop", "{s:s}", "group", group);
}
