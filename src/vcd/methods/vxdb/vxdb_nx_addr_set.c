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

#include "lucid.h"

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

xmlrpc_value *m_vxdb_nx_addr_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	char *name, *addr, *netm, *bcas;
	xid_t xid;
	dbi_result dbr;
	
	params = method_init(env, p, VCD_CAP_NET, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,s:s,s:s,*}",
		"name", &name,
		"addr", &addr,
		"netmask", &netm,
		"broadcast", &bcas);
	method_return_if_fault(env);
	
	method_empty_params(2, &netm, &bcas);
	
	if (!validate_name(name) || !validate_addr(addr) ||
	   (netm && !validate_addr(netm)) ||
	   (bcas && !validate_addr(bcas)))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	if (!netm)
		netm = "";
	
	if (!bcas)
		bcas = "";
	
	dbr = dbi_conn_queryf(vxdb,
		"INSERT OR REPLACE INTO nx_addr (xid, addr, netmask, broadcast) "
		"VALUES (%d, '%s', '%s', '%s')",
		xid, addr, netm, bcas);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	return xmlrpc_nil_new(env);
}
