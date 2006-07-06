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
#include <stdio.h>

#include "cmd.h"

void cmd_init_method_get(xmlrpc_env *env, int argc, char **argv)
{
	char *method, *start, *stop;
	int timeout;
	xmlrpc_value *response;
	
	response = xmlrpc_client_call(env, uri, "vxdb.init.method.get",
		SIGNATURE("{s:s}"),
		"name", name);
	return_if_fault(env);
	
	xmlrpc_decompose_value(env, response,
		"{s:s,s:s,s:s,s:i,*}",
		"method", &method,
		"start", &start,
		"stop", &stop,
		"timeout", &timeout);
	return_if_fault(env);
	
	xmlrpc_DECREF(response);
	
	printf("METHOD=%s\n", method);
	printf("START=%s\n", start);
	printf("STOP=%s\n", stop);
	printf("TIMEOUT=%d\n", timeout);
}

void cmd_init_method_set(xmlrpc_env *env, int argc, char **argv)
{
	char *method, *start, *stop;
	int timeout;
	xmlrpc_value *response;
	
	if (argc < 1)
		usage(EXIT_FAILURE);
	
	method = argv[0];
	
	if (argc < 4) {
		response = xmlrpc_client_call(env, uri, "vxdb.init.method.get",
			SIGNATURE("{s:s}"),
			"name", name);
		return_if_fault(env);
		
		xmlrpc_decompose_value(env, response,
			"{s:s,s:s,s:i,*}",
			"start", &start,
			"stop", &stop,
			"timeout", &timeout);
		return_if_fault(env);
		
		xmlrpc_DECREF(response);
	}
	
	if (argc > 1)
		timeout = atoi(argv[1]);
	
	if (argc > 2)
		start = argv[2];
	
	if (argc > 3)
		stop = argv[3];
	
	xmlrpc_client_call(env, uri, "vxdb.init.method.set",
		SIGNATURE("{s:s,s:s,s:s,s:s,s:i}"),
		"name", name,
		"method", method,
		"start", start,
		"stop", stop,
		"timeout", timeout);
}
