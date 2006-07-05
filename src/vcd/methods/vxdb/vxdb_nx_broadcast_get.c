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

xmlrpc_value *m_vxdb_nx_broadcast_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	char *name;
	xid_t xid;
	dbi_result dbr;
	
	params = method_init(env, p, VCD_CAP_NET, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,*}",
		"name", &name);
	method_return_if_fault(env);
	
	if (!validate_name(name))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT broadcast FROM nx_broadcast "
		"WHERE xid = %d",
		xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	if (dbi_result_get_numrows(dbr) < 1)
		return xmlrpc_build_value(env, "s", "");
	
	dbi_result_first_row(dbr);
	
	return xmlrpc_build_value(env, "s", dbi_result_get_string(dbr, "broadcast"));
}
