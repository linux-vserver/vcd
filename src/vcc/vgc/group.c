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

void cmd_grouplist(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *response, *result;
	char *groupname;
	int len, i, gid;

	response = client_call0("vg.list");
	return_if_fault(env);

	len = xmlrpc_array_size(env, response);
	return_if_fault(env);

	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);

		xmlrpc_decompose_value(env, result,
			"{s:s,s:i,*}",
			"groupname", &groupname,
			"gid", &gid);
		return_if_fault(env);

		xmlrpc_DECREF(result);

		printf("%s (gid=%d)\n", groupname, gid);
	}

	xmlrpc_DECREF(response);
}

void cmd_groupadd(xmlrpc_env *env, int argc, char **argv)
{
	client_call("vg.add",
		"{s:s}",
		"groupname", name);
}

void cmd_groupremove(xmlrpc_env *env, int argc, char **argv)
{
	client_call("vg.remove",
		"{s:s}",
		"groupname", name);
}
