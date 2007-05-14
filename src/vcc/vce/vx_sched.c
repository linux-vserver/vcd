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

void cmd_vx_sched_get(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *response, *result;
	char *name;
	int cpuid, interval, fillrate, interval2, fillrate2;
	int tokensmin, tokensmax;
	int i, len;

	if (argc < 1)
		usage(EXIT_FAILURE);

	name = argv[0];

	if (argc > 1)
		sscanf(argv[1], "%d", &cpuid);
	else
		cpuid = -2;

	response = client_call("vxdb.vx.sched.get",
		"{s:s,s:i}",
		"name", name,
		"cpuid", cpuid);
	return_if_fault(env);

	len = xmlrpc_array_size(env, response);
	return_if_fault(env);

	for (i = 0; i < len; i++) {
		xmlrpc_array_read_item(env, response, i, &result);
		return_if_fault(env);

		xmlrpc_decompose_value(env, result,
			"{s:i,s:i,s:i,s:i,s:i,s:i,s:i,*}",
			"cpuid", &cpuid,
			"interval", &interval,
			"fillrate", &fillrate,
			"interval2", &interval2,
			"fillrate2", &fillrate2,
			"tokensmin", &tokensmin,
			"tokensmax", &tokensmax);
		return_if_fault(env);

		xmlrpc_DECREF(result);

		printf("%d: %d %d %d %d %d %d\n", cpuid, interval, fillrate,
			interval2, fillrate2, tokensmin, tokensmax);
	}

	xmlrpc_DECREF(response);
}

void cmd_vx_sched_remove(xmlrpc_env *env, int argc, char **argv)
{
	char *name;
	int cpuid;

	if (argc < 1)
		usage(EXIT_FAILURE);

	name = argv[0];

	if (argc > 1)
		sscanf(argv[1], "%d", &cpuid);
	else
		cpuid = -2;

	client_call("vxdb.vx.sched.remove",
		"{s:s,s:i}",
		"name", name,
		"cpuid", cpuid);
}

void cmd_vx_sched_set(xmlrpc_env *env, int argc, char **argv)
{
	char *name;
	int cpuid, interval, fillrate, interval2, fillrate2;
	int tokensmin, tokensmax;

	if (argc < 8)
		usage(EXIT_FAILURE);

	name = argv[0];
	sscanf(argv[1], "%d", &cpuid);
	sscanf(argv[2], "%d", &interval);
	sscanf(argv[3], "%d", &fillrate);
	sscanf(argv[4], "%d", &interval2);
	sscanf(argv[5], "%d", &fillrate2);
	sscanf(argv[6], "%d", &tokensmin);
	sscanf(argv[7], "%d", &tokensmax);

	client_call("vxdb.vx.sched.set",
		"{s:s,s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
		"name", name,
		"cpuid", cpuid,
		"interval", interval,
		"fillrate", fillrate,
		"interval2", interval2,
		"fillrate2", fillrate2,
		"tokensmin", tokensmin,
		"tokensmax", tokensmax);
}
