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

static
struct limit_data {
	int id;
	char *db;
} LIMIT[] = {
	{ RLIMIT_AS,       "mem_AS"       },
	{ RLIMIT_LOCKS,    "file_LOCKS"   },
	{ RLIMIT_MEMLOCK,  "mem_MEMLOCK"  },
	{ RLIMIT_MSGQUEUE, "ipc_MSGQUEUE" },
	{ RLIMIT_NOFILE,   "file_NOFILE"  },
	{ RLIMIT_NPROC,    "sys_NPROC"    },
	{ RLIMIT_RSS,      "mem_RSS"      },
	{ VLIMIT_ANON,     "mem_ANON"     },
	{ VLIMIT_DENTRY,   "file_DENTRY"  },
	{ VLIMIT_MAPPED,   "sys_MAPPED"   },
	{ VLIMIT_NSEMS,    "ipc_NSEMS"    },
	{ VLIMIT_NSOCK,    "file_NSOCK"   },
	{ VLIMIT_OPENFD,   "file_OPENFD"  },
	{ VLIMIT_SEMARY,   "ipc_SEMARY"   },
	{ VLIMIT_SHMEM,    "ipc_SHMEM"    },
	{ 0,               NULL           }
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

			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:i,s:i,s:i}",
				"type", LIMIT[i].db,
				"min",  (int) limstat.minimum,
				"cur",  (int) limstat.value,
				"max",  (int) limstat.maximum);
		}
	}

	return response;
}
