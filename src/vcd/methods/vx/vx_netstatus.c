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

#include <lucid/log.h>

static
struct net_data {
	int id;
	char *db;
} NET[] = {
	{ NXA_SOCK_UNSPEC, "net_UNSPEC" },
	{ NXA_SOCK_UNIX,   "net_UNIX"   },
	{ NXA_SOCK_INET,   "net_INET"   },
	{ NXA_SOCK_INET6,  "net_INET6"  },
	{ NXA_SOCK_PACKET, "net_PACKET" },
	{ NXA_SOCK_OTHER,  "net_OTHER"  },
	{ 0,               NULL         }
};

/* vx.netstatus(string name) */
xmlrpc_value *m_vx_netstatus(xmlrpc_env *env, xmlrpc_value *p, void *c)
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

		for (i = 0; NET[i].db; i++) {
			nx_sock_stat_t netstat;

			netstat.id = NET[i].id;

			if (nx_sock_stat(xid, &netstat) == -1) {
				log_perror("nx_sock_stat(%s, %d)", NET[i].db, xid);
			}

			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:i,s:i,s:i,s:i,s:i,s:i}",
				"type",  NET[i].db,
				"recvp", (int) netstat.count[0],
				"recvb", (int) netstat.total[0],
				"sendp", (int) netstat.count[1],
				"sendb", (int) netstat.total[1],
				"failp", (int) netstat.count[2],
				"failb", (int) netstat.total[2]);
		}
	}

	return response;
}
