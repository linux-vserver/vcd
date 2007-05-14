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

#include <lucid/printf.h>

#include "vcc.h"
#include "cmd.h"

static
char *pretty_uptime(int sec)
{
	char *buf = NULL;
	int d = 0, h = 0, m = 0, s = 0;

	s = sec % 60;
	sec /= 60;
	m = sec % 60;
	sec /= 60;
	h = sec % 24;
	sec /= 24;
	d = sec;

	if (d > 0)
		asprintf(&buf, "%4dd%02dh%02dm%02ds", d, h, m, s);
	else if (h > 0)
		asprintf(&buf, "%2dh%02dm%02ds", h, m, s);
	else
		asprintf(&buf, "%2dm%02ds", m, s);

	return buf;
}

void cmd_status(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *result;
	int uptime = 0, requests = 0, nomethod = 0, flogins = 0;

	result = client_call0("vcd.status");
	return_if_fault(env);

	xmlrpc_decompose_value(env, result,
		"{s:i,s:i,s:i,s:i,*}",
		"uptime", &uptime,
		"requests", &requests,
		"nomethod", &nomethod,
		"flogins", &flogins);
	return_if_fault(env);

	xmlrpc_DECREF(result);

	printf("uptime: %s\n", pretty_uptime(uptime));
	printf("requests: %d\n", requests);
	printf("nomethod: %d\n", nomethod);
	printf("flogins:  %d\n", flogins);
}
