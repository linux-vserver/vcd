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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <vserver.h>
#include <lucid/addr.h>

#include "auth.h"
#include <lucid/log.h>
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* netup process:
   1) add ip addresses
   2) set broadcast
*/

static
xmlrpc_value *network_interfaces(xmlrpc_env *env, xid_t xid)
{
	LOG_TRACEME
	
	int rc;
	vxdb_result *dbr;
	const char *ip, *netm;
	char buf[32];
	nx_addr_t addr;
	
	rc = vxdb_prepare(&dbr,
		"SELECT addr,netmask FROM nx_addr WHERE xid = %d",
		xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else {
		vxdb_foreach_step(rc, dbr) {
			ip   = sqlite3_column_text(dbr, 0);
			netm = sqlite3_column_text(dbr, 1);
			
			bzero(buf, 32);
			snprintf(buf, 31, "%s/%s", ip, netm);
			
			addr.type  = NXA_TYPE_IPV4;
			addr.count = 1;
			
			if (addr_from_str(buf, &addr.ip[0], &addr.mask[0]) == -1) {
				method_set_faultf(env, MECONF, "invalid interface: %s", buf);
				break;
			}
			
			else if (nx_addr_add(xid, &addr) == -1) {
				method_set_faultf(env, MESYS, "nx_add_addr: %s", strerror(errno));
				break;
			}
		}
		
		if (rc == -1)
			method_set_fault(env, MEVXDB);
	}
	
	sqlite3_finalize(dbr);
	
	return NULL;
}

static
xmlrpc_value *network_broadcast(xmlrpc_env *env, xid_t xid)
{
	LOG_TRACEME

	int rc;
	vxdb_result *dbr;
	const char *ip;
	char buf[32];
	nx_addr_t addr;
	
	rc = vxdb_prepare(&dbr,
		"SELECT broadcast FROM nx_broadcast WHERE xid = %d",
		xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else {
		rc = vxdb_step(dbr);
		
		if (rc == -1)
			method_set_fault(env, MEVXDB);
		
		else if (rc == 1) {
			addr.type  = NXA_TYPE_IPV4 | NXA_MOD_BCAST;
			addr.count = 1;
			
			ip = sqlite3_column_text(dbr, 0);
			
			if (addr_from_str(ip, &addr.ip[0], &addr.mask[0]) == -1)
				method_set_faultf(env, MECONF, "invalid interface: %s", buf);
			
			else {
				addr.mask[0] = 0;
				
				if (nx_addr_add(xid, &addr) == -1)
					method_set_faultf(env, MESYS, "nx_add_addr: %s", strerror(errno));
			}
		}
	}
	
	sqlite3_finalize(dbr);
	
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
