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

void cmd_vx_sched_get(xmlrpc_env *env, int argc, char **argv)
{
	int cpuid, interval, fillrate, interval2, fillrate2;
	int tokensmin, tokensmax;
	xmlrpc_value *response, *result;
	int i, len;

	if (argc < 1)
		cpuid = -2;
	else
		sscanf(argv[0], "%d", &cpuid);

	response = xmlrpc_client_call(env, uri, "vxdb.vx.sched.get",
		SIGNATURE("{s:s,s:i}"),
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

		printf("%d: %d %d %d %d %d %d\n", cpuid, interval, fillrate, interval2,
			fillrate2, tokensmin, tokensmax);
	}

	xmlrpc_DECREF(response);
}

void cmd_vx_sched_remove(xmlrpc_env *env, int argc, char **argv)
{
	int cpuid;

	if (argc < 1)
		cpuid = -2;
	else
		sscanf(argv[0], "%d", &cpuid);

	xmlrpc_client_call(env, uri, "vxdb.vx.sched.remove",
		SIGNATURE("{s:s,s:i}"),
		"name", name,
		"cpuid", cpuid);
}

void cmd_vx_sched_set(xmlrpc_env *env, int argc, char **argv)
{
	int cpuid = -1, interval, fillrate, interval2, fillrate2;
	int tokensmin, tokensmax, i = 0;

	if (argc < 6)
		usage(EXIT_FAILURE);

	if (argc > 6)
		sscanf(argv[i++], "%d", &cpuid);

	sscanf(argv[i++], "%d", &interval);
	sscanf(argv[i++], "%d", &fillrate);
	sscanf(argv[i++], "%d", &interval2);
	sscanf(argv[i++], "%d", &fillrate2);
	sscanf(argv[i++], "%d", &tokensmin);
	sscanf(argv[i++], "%d", &tokensmax);

	xmlrpc_client_call(env, uri, "vxdb.vx.sched.set",
		SIGNATURE("{s:s,s:i,s:i,s:i,s:i,s:i,s:i,s:i}"),
		"name", name,
		"cpuid", cpuid,
		"interval", interval,
		"fillrate", fillrate,
		"interval2", interval2,
		"fillrate2", fillrate2,
		"tokensmin", tokensmin,
		"tokensmax", tokensmax);
}
