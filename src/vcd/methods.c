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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>

#include "xmlrpc.h"
#include "xmlrpc_private.h"

#include "auth.h"
#include "methods.h"

m_err_t method_error_codes[] = {
	{ MEAUTH,    "Unauthorized" },
	{ MEPERM,    "Operation not permitted" },
	{ MEREQ,     "Invalid request" },
	{ MEVXDB,    "Error in vxdb" },
	{ MECONF,    "Invalid configuration" },
	{ MESTOPPED, "Not running" },
	{ MERUNNING, "Already running" },
	{ MEEXIST,   "Conflict/Already exists" },
	{ MENOENT,   "Not found" },
	{ MESYS,     "System call failed" },
	{ 0,         NULL },
};

#define MREGISTER(NAME,FUNC)  XMLRPC_ServerRegisterMethod(s, NAME, FUNC)

void method_registry_init(XMLRPC_SERVER s)
{
	/* vx */
	MREGISTER("vx.killer", m_vx_killer);
	MREGISTER("vx.restart", m_vx_restart);
	MREGISTER("vx.start", m_vx_start);
	MREGISTER("vx.stop", m_vx_stop);
	
	/* vxdb */
	MREGISTER("vxdb.create", m_vxdb_create);
	MREGISTER("vxdb.dx.limit.get", m_vxdb_dx_limit_get);
	MREGISTER("vxdb.dx.limit.remove", m_vxdb_dx_limit_remove);
	MREGISTER("vxdb.dx.limit.set", m_vxdb_dx_limit_set);
	MREGISTER("vxdb.init.method.get", m_vxdb_init_method_get);
	MREGISTER("vxdb.init.method.remove", m_vxdb_init_method_remove);
	MREGISTER("vxdb.init.method.set", m_vxdb_init_method_set);
	MREGISTER("vxdb.init.mount.add", m_vxdb_init_mount_add);
	MREGISTER("vxdb.init.mount.get", m_vxdb_init_mount_get);
	MREGISTER("vxdb.init.mount.remove", m_vxdb_init_mount_remove);
	MREGISTER("vxdb.list", m_vxdb_list);
	MREGISTER("vxdb.nx.addr.add", m_vxdb_nx_addr_add);
	MREGISTER("vxdb.nx.addr.get", m_vxdb_nx_addr_get);
	MREGISTER("vxdb.nx.addr.remove", m_vxdb_nx_addr_remove);
	MREGISTER("vxdb.owner.add", m_vxdb_owner_add);
	MREGISTER("vxdb.owner.get", m_vxdb_owner_get);
	MREGISTER("vxdb.owner.remove", m_vxdb_owner_remove);
	MREGISTER("vxdb.remove", m_vxdb_remove);
	MREGISTER("vxdb.user.add", m_vxdb_user_add);
	MREGISTER("vxdb.user.get", m_vxdb_user_get);
	MREGISTER("vxdb.user.remove", m_vxdb_user_remove);
	MREGISTER("vxdb.user.set", m_vxdb_user_set);
	MREGISTER("vxdb.vx.bcaps.add", m_vxdb_vx_bcaps_add);
	MREGISTER("vxdb.vx.bcaps.get", m_vxdb_vx_bcaps_get);
	MREGISTER("vxdb.vx.bcaps.remove", m_vxdb_vx_bcaps_remove);
	MREGISTER("vxdb.vx.ccaps.add", m_vxdb_vx_ccaps_add);
	MREGISTER("vxdb.vx.ccaps.get", m_vxdb_vx_ccaps_get);
	MREGISTER("vxdb.vx.ccaps.remove", m_vxdb_vx_ccaps_remove);
	MREGISTER("vxdb.vx.flags.add", m_vxdb_vx_flags_add);
	MREGISTER("vxdb.vx.flags.get", m_vxdb_vx_flags_get);
	MREGISTER("vxdb.vx.flags.remove", m_vxdb_vx_flags_remove);
	MREGISTER("vxdb.vx.limit.get", m_vxdb_vx_limit_get);
	MREGISTER("vxdb.vx.limit.remove", m_vxdb_vx_limit_remove);
	MREGISTER("vxdb.vx.limit.set", m_vxdb_vx_limit_set);
	MREGISTER("vxdb.vx.pflags.add", m_vxdb_vx_pflags_add);
	MREGISTER("vxdb.vx.pflags.get", m_vxdb_vx_pflags_get);
	MREGISTER("vxdb.vx.pflags.remove", m_vxdb_vx_pflags_remove);
	MREGISTER("vxdb.vx.sched.get", m_vxdb_vx_sched_get);
	MREGISTER("vxdb.vx.sched.remove", m_vxdb_vx_sched_remove);
	MREGISTER("vxdb.vx.sched.set", m_vxdb_vx_sched_set);
	MREGISTER("vxdb.vx.uname.get", m_vxdb_vx_uname_get);
	MREGISTER("vxdb.vx.uname.remove", m_vxdb_vx_uname_remove);
	MREGISTER("vxdb.vx.uname.set", m_vxdb_vx_uname_set);
}

#undef MREGISTER

XMLRPC_VALUE method_call(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_Callback cb;
	
	if (r->error)
		return XMLRPC_CopyValue(r->error);
	
	if (!auth_isvalid(r))
		return method_error(MEAUTH);
	
	cb = XMLRPC_ServerFindMethod(s, r->methodName.str);
	
	if (cb)
		return cb(s, r, d);
	
	return XMLRPC_UtilityCreateFault(xmlrpc_error_unknown_method, r->methodName.str);
}

XMLRPC_VALUE method_get_params(XMLRPC_REQUEST r)
{
	XMLRPC_VALUE request, auth, params;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	params  = XMLRPC_VectorNext(request);
	
	return params;
}

char *method_strerror(int id)
{
	int i;
	
	for (i = 0; method_error_codes[i].msg; i++)
		if (method_error_codes[i].id == id)
			return method_error_codes[i].msg;
	
	return NULL;
}

XMLRPC_VALUE method_error(int id)
{
	return XMLRPC_UtilityCreateFault(id, method_strerror(id));
}
