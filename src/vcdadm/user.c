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

void cmd_user_get(xmlrpc_env *env, int argc, char **argv)
{
	char *username;
	xmlrpc_value *response, *result;
	int len, i, uid, admin;

	response = xmlrpc_client_call(env, uri, "vcd.user.get",
		SIGNATURE("{s:s}"),
		"username", name);
	return_if_fault(env);

	len = xmlrpc_array_size(env, response);
	return_if_fault(env);

	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);
		
		xmlrpc_decompose_value(env, result,
			"{s:s,s:i,s:i,*}",
			"username", &username,
			"uid", &uid,
			"admin", &admin);
		return_if_fault(env);
		
		xmlrpc_DECREF(result);
		
		printf("%s (uid=%d,admin=%d)", username, uid, admin ? 1 : 0);
	}

	xmlrpc_DECREF(response);
}

void cmd_user_remove(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_client_call(env, uri, "vcd.user.remove",
		SIGNATURE("{s:s}"),
		"username", name);
}

void cmd_user_set(xmlrpc_env *env, int argc, char **argv)
{
	char *password;
	int admin;

	if (argc < 1)
		usage(EXIT_FAILURE);

	sscanf(argv[0], "%d", &admin);

	if (argc > 1)
		password = argv[1];
	else
		password = "";

	xmlrpc_client_call(env, uri, "vcd.user.set",
		SIGNATURE("{s:s,s:i,s:s}"),
		"username", name,
		"admin", admin,
		"password", password);
}
