// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
//           2007 Luca Longinotti <chtekk@gentoo.org>
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
	char *name, *load1m, *load5m, *load15m;
	int running = 0, thr_total = 0, thr_running = 0, thr_unintr = 0, thr_onhold = 0;
	int forks_total = 0, uptime = 0;

	if (argc < 1)
		usage(EXIT_FAILURE);

	name = argv[0];

	result = client_call("vx.status",
		"{s:s}",
		"name", name);
	return_if_fault(env);

	xmlrpc_decompose_value(env, result,
		"{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:s,s:s,s:s,*}",
		"running",     &running,
		"thr_total",   &thr_total,
		"thr_running", &thr_running,
		"thr_unintr",  &thr_unintr,
		"thr_onhold",  &thr_onhold,
		"forks_total", &forks_total,
		"uptime",      &uptime,
		"load1m",      &load1m,
		"load5m",      &load5m,
		"load15m",     &load15m);
	return_if_fault(env);

	xmlrpc_DECREF(result);

	if (running == 0) {
		printf("running: %d\n", running);
	}
	else {
		printf("running: %d\n", running);
		printf("total threads: %d\n", thr_total);
		printf("(running: %d, unintr: %d, onhold: %d)\n",
				thr_running, thr_unintr, thr_onhold);
		printf("total forks: %d\n", forks_total);
		printf("uptime: %s\n", pretty_uptime(uptime));
		printf("load average: %s, %s, %s\n", load1m, load5m, load15m);
	}
}
