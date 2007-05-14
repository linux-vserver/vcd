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

#include <unistd.h>
#include <vserver.h>
#include <sys/resource.h>

#include <lucid/log.h>

static
int pagestobytes(int pages) {
	int bytes;
	bytes = (pages * getpagesize()) >> 10;
	return bytes;
}

static
struct limit_data {
	int id;
	char *db;
	int pagestobytes;
} LIMIT[] = {
	{ RLIMIT_AS,       "mem_AS",       1 },
	{ RLIMIT_LOCKS,    "file_LOCKS",   0 },
	{ RLIMIT_MEMLOCK,  "mem_MEMLOCK",  1 },
	{ RLIMIT_MSGQUEUE, "ipc_MSGQUEUE", 0 },
	{ RLIMIT_NOFILE,   "file_NOFILE",  0 },
	{ RLIMIT_NPROC,    "sys_NPROC",    0 },
	{ RLIMIT_RSS,      "mem_RSS",      1 },
	{ VLIMIT_ANON,     "mem_ANON",     1 },
	{ VLIMIT_DENTRY,   "file_DENTRY",  0 },
	{ VLIMIT_MAPPED,   "sys_MAPPED",   0 },
	{ VLIMIT_NSEMS,    "ipc_NSEMS",    0 },
	{ VLIMIT_NSOCK,    "file_NSOCK",   0 },
	{ VLIMIT_OPENFD,   "file_OPENFD",  0 },
	{ VLIMIT_SEMARY,   "ipc_SEMARY",   0 },
	{ VLIMIT_SHMEM,    "ipc_SHMEM",    0 },
	{ 0,               NULL,           0 }
};

/* vx.limstatus(string name) */
xmlrpc_value *m_vx_limstatus(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params, *response = NULL;
	char *name;
	xid_t xid;

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
			method_return_fault(env, MESTOPPED);
		}

		else
			method_return_sys_fault(env, "vx_info");
	}

	else {
		response = xmlrpc_array_new(env);

		int i;

		for (i = 0; LIMIT[i].db; i++) {
			vx_limit_stat_t limstat;

			limstat.id = LIMIT[i].id;

			if (vx_limit_stat(xid, &limstat) == -1) {
				log_perror("vx_limit_stat(%s, %d)", LIMIT[i].db, xid);
			}

			int min = 0, cur = 0, max = 0;

			if (LIMIT[i].pagestobytes == 1) {
				min = pagestobytes((int) limstat.minimum);
				cur = pagestobytes((int) limstat.value);
				max = pagestobytes((int) limstat.maximum);
			}

			else {
				min = (int) limstat.minimum;
				cur = (int) limstat.value;
				max = (int) limstat.maximum;
			}

			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:i,s:i,s:i}",
				"type", LIMIT[i].db,
				"min",  min,
				"cur",  cur,
				"max",  max));
		}
	}

	return response;
}
