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

#include "vcc.h"
#include "cmd.h"

void cmd_nx_addr_get(xmlrpc_env *env, int argc, char **argv)
{
	char *addr, *netmask;
	xmlrpc_value *response, *result;
	int len, i;

	if (argc < 1)
		usage(EXIT_FAILURE);

	char *name = argv[0];

	if (argc < 2)
		addr = "";
	else
		addr = argv[1];

	response = client_call("vxdb.nx.addr.get", "{s:s,s:s}",
		"name", name,
		"addr", addr);
	return_if_fault(env);

	len = xmlrpc_array_size(env, response);
	return_if_fault(env);

	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);

		xmlrpc_decompose_value(env, result,
			"{s:s,s:s,*}",
			"addr", &addr,
			"netmask", &netmask);
		return_if_fault(env);

		xmlrpc_DECREF(result);

		printf("%s/%s\n", addr, netmask);
	}

	xmlrpc_DECREF(response);
}

void cmd_nx_addr_remove(xmlrpc_env *env, int argc, char **argv)
{
	char *addr;

	if (argc < 1)
		usage(EXIT_FAILURE);

	char *name = argv[0];

	if (argc < 2)
		addr = "";
	else
		addr = argv[1];

	client_call("vxdb.nx.addr.remove", "{s:s,s:s}",
		"name", name,
		"addr", addr);
}

void cmd_nx_addr_set(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *response, *result;
	char *name, *addr, *netmask = "255.255.255.255";

	if (argc < 2)
		usage(EXIT_FAILURE);

	name = argv[0];
	addr = argv[1];

	response = client_call("vxdb.nx.addr.get", "{s:s,s:s}",
		"name", name,
		"addr", addr);
	return_if_fault(env);

	if (xmlrpc_array_size(env, response) > 0) {
		xmlrpc_array_read_item(env, response, 0, &result);
		return_if_fault(env);

		xmlrpc_decompose_value(env, result,
			"{s:s,*}",
			"netmask", &netmask);
		return_if_fault(env);

		xmlrpc_DECREF(result);
		xmlrpc_DECREF(response);
	}

	if (argc > 1)
		netmask = argv[1];

	client_call("vxdb.nx.addr.set", "{s:s,s:s,s:s}",
		"name", name,
		"addr", addr,
		"netmask", netmask);
}
