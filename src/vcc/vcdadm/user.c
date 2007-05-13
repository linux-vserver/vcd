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

#include <lucid/printf.h>
#include <lucid/scanf.h>

#include "vcc.h"
#include "cmd.h"

void cmd_user_get(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *response, *result;
	char *username;
	int len, i, uid, admin;

	if (argc > 0)
		username = argv[0];
	else
		username = "";

	response = client_call("vcd.user.get", "{s:s}",
		"username", username);
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

		printf("%s (uid=%d,admin=%d)\n", username, uid, admin ? 1 : 0);
	}

	xmlrpc_DECREF(response);
}

void cmd_user_remove(xmlrpc_env *env, int argc, char **argv)
{
	char *username;

	if (argc < 1)
		usage(EXIT_FAILURE);

	username = argv[0];

	client_call("vcd.user.remove", "{s:s}",
		"username", username);
}

void cmd_user_set(xmlrpc_env *env, int argc, char **argv)
{
	char *username, *password;
	int admin;

	if (argc < 2)
		usage(EXIT_FAILURE);

	username = argv[0];

	sscanf(argv[1], "%d", &admin);

	if (argc > 2)
		password = argv[2];
	else
		password = "";

	client_call("vcd.user.set", "{s:s,s:s,s:b}",
		"username", username,
		"password", password,
		"admin", admin);
}
