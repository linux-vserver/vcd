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

#include "auth.h"
#include "methods.h"
#include "vxdb.h"

#include <lucid/addr.h>
#include <lucid/log.h>
#include <lucid/mem.h>
#include <lucid/printf.h>

/* netup process:
   1) add ip addresses
   2) set broadcast
*/

static
xmlrpc_value *network_interfaces(xmlrpc_env *env, xid_t xid)
{
	LOG_TRACEME

	int rc;

	rc = vxdb_prepare(&dbr,
			"SELECT addr,netmask FROM nx_addr WHERE xid = %d",
			xid);

	if (rc == VXDB_OK) {
		vxdb_foreach_step(rc, dbr) {
			const char *ip   = vxdb_column_text(dbr, 0);
			const char *netm = vxdb_column_text(dbr, 1);

			char buf[32];
			mem_set(buf, 0, 32);
			snprintf(buf, 31, "%s/%s", ip, netm);

			nx_addr_t addr = {
				.type  = NXA_TYPE_IPV4,
				.count = 1,
			};

			if (addr_from_str(buf, &addr.ip[0], &addr.mask[0]) == -1) {
				method_set_faultf(env, MECONF, "invalid interface: %s", buf);
				break;
			}

			log_debug("addr(%d): %#.8lx/%#.8lx", xid, addr.ip, addr.mask);

			if (nx_addr_add(xid, &addr) == -1) {
				method_set_sys_fault(env, "nx_add_addr");
				break;
			}
		}
	}

	if (!env->fault_occurred && rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);
	return NULL;
}

static
xmlrpc_value *network_broadcast(xmlrpc_env *env, xid_t xid)
{
	LOG_TRACEME

	int rc;

	rc = vxdb_prepare(&dbr,
			"SELECT broadcast FROM nx_broadcast WHERE xid = %d",
			xid);

	if (rc == VXDB_OK) {
		vxdb_foreach_step(rc, dbr) {
			nx_addr_t addr = {
				.type  = NXA_TYPE_IPV4 | NXA_MOD_BCAST,
				.count = 1,
				.mask  = { 0, 0, 0, 0 },
			};

			const char *ip = vxdb_column_text(dbr, 0);

			if (addr_from_str(ip, &addr.ip[0], NULL) != 1)
				method_set_faultf(env, MECONF, "invalid broadcast: %s", ip);

			else {
				log_debug("broadcast(%d): %#.8lx", xid, addr.ip);

				if (nx_addr_add(xid, &addr) == -1)
					method_set_sys_fault(env, "nx_add_addr");
			}
		}
	}

	if (!env->fault_occurred && rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);
	return NULL;
}

/* helper.netup(int xid) */
xmlrpc_value *m_helper_netup(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_HELPER, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:i,*}",
			"xid", &xid);
	method_return_if_fault(env);

	if (!vxdb_getname(xid))
		method_return_fault(env, MENOVPS);

	if (nx_info(xid, NULL) == -1)
		method_return_fault(env, MESTOPPED);

	network_interfaces(env, xid);
	method_return_if_fault(env);

	network_broadcast(env, xid);
	method_return_if_fault(env);

	return xmlrpc_nil_new(env);
}
