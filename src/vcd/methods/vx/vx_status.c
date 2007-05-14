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

#include "auth.h"
#include "methods.h"
#include "vxdb.h"

#include <vserver.h>
#include <sys/resource.h>

#include <lucid/log.h>
#include <lucid/printf.h>

static
char *pretty_load(int load)
{
	char *buf = NULL;

	asprintf(&buf, "%d.%02d", load >> 11, ((load & ((1 << 11) - 1)) * 100) >> 11);

	return buf;
}

/* vx.status(string name) */
xmlrpc_value *m_vx_status(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params, *response = NULL;
	char *name;
	xid_t xid;
	int running = 0, nproc = 0, tasks = 0, thr_total = 0, thr_running = 0;
	int thr_unintr = 0, thr_onhold = 0, forks_total = 0, uptime = 0;
	char *load1m = NULL, *load5m = NULL, *load15m = NULL;

	params = method_init(env, p, c, VCD_CAP_INFO, M_OWNER);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,*}",
			"name", &name);
	method_return_if_fault(env);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	if (vx_info(xid, NULL) == -1) {
		if (errno == ESRCH) {
			running     = 0;
			nproc       = 0;
			tasks       = 0;
			thr_total   = 0;
			thr_running = 0;
			thr_unintr  = 0;
			thr_onhold  = 0;
			forks_total = 0;
			uptime      = 0;
			load1m      = "0";
			load5m      = "0";
			load15m     = "0";
		}

		else
			method_return_sys_fault(env, "vx_info");
	}

	else {
		vx_stat_t statb;
		vx_limit_stat_t limnproc;

		limnproc.id = RLIMIT_NPROC;

		if (vx_stat(xid, &statb) == -1)
			log_perror("vx_stat(%d)", xid);

		else if (vx_limit_stat(xid, &limnproc) == -1)
			log_perror("vx_limit_stat(NPROC, %d)", xid);

		else {
			running     = 1;
			nproc       = (int) limnproc.value;
			tasks       = statb.tasks;
			thr_total   = statb.nr_threads;
			thr_running = statb.nr_running;
			thr_unintr  = statb.nr_unintr;
			thr_onhold  = statb.nr_onhold;
			forks_total = statb.nr_forks;
			uptime      = statb.uptime/1000000000;
			load1m      = pretty_load(statb.load[0]);
			load5m      = pretty_load(statb.load[1]);
			load15m     = pretty_load(statb.load[2]);
		}
	}

	response = xmlrpc_build_value(env,
		"{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:s,s:s,s:s}",
		"running",     running,
		"nproc",       nproc,
		"tasks",       tasks,
		"thr_total",   thr_total,
		"thr_running", thr_running,
		"thr_unintr",  thr_unintr,
		"thr_onhold",  thr_onhold,
		"forks_total", forks_total,
		"uptime",      uptime,
		"load1m",      load1m,
		"load5m",      load5m,
		"load15m",     load15m);

	return response;
}
