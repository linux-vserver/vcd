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

#include <string.h>

#include "auth.h"
#include "methods.h"

m_err_t method_error_codes[] = {
	{ MEAUTH,    "Unauthorized" },
	{ MEPERM,    "Operation not permitted" },
	{ MEINVAL,   "Invalid request" },
	{ MEVXDB,    "Error in vxdb" },
	{ MECONF,    "Invalid configuration" },
	{ MESTOPPED, "Not running" },
	{ MERUNNING, "Already running" },
	{ MEEXIST,   "Conflict/Already exists" },
	{ MENOVPS,   "Not found" },
	{ MESYS,     "System call failed" },
	{ 0,         NULL },
};

#define MREGISTER(NAME,FUNC) do { \
	xmlrpc_registry_add_method(env, registry, NULL, NAME, &FUNC, NULL); \
} while (0)

int method_registry_init(xmlrpc_env *env, xmlrpc_registry *registry)
{
	/* vx */
/*
	MREGISTER("vx.create",  m_vx_create);
	MREGISTER("vx.killer",  m_vx_killer);
	MREGISTER("vx.restart", m_vx_restart);
	MREGISTER("vx.start",   m_vx_start);
	MREGISTER("vx.stop",    m_vx_stop);
*/
	
	/* vxdb */
	MREGISTER("vxdb.dx.limit.get",     m_vxdb_dx_limit_get);
	MREGISTER("vxdb.dx.limit.remove",  m_vxdb_dx_limit_remove);
	MREGISTER("vxdb.dx.limit.set",     m_vxdb_dx_limit_set);
	MREGISTER("vxdb.init.method.get",  m_vxdb_init_method_get);
	MREGISTER("vxdb.init.method.set",  m_vxdb_init_method_set);
	MREGISTER("vxdb.list",             m_vxdb_list);
	MREGISTER("vxdb.mount.get",        m_vxdb_mount_get);
	MREGISTER("vxdb.mount.remove",     m_vxdb_mount_remove);
	MREGISTER("vxdb.mount.set",        m_vxdb_mount_set);
	MREGISTER("vxdb.name.get",         m_vxdb_name_get);
	MREGISTER("vxdb.nx.addr.get",      m_vxdb_nx_addr_get);
	MREGISTER("vxdb.nx.addr.remove",   m_vxdb_nx_addr_remove);
	MREGISTER("vxdb.nx.addr.set",      m_vxdb_nx_addr_set);
	MREGISTER("vxdb.owner.add",        m_vxdb_owner_add);
	MREGISTER("vxdb.owner.get",        m_vxdb_owner_get);
	MREGISTER("vxdb.owner.remove",     m_vxdb_owner_remove);
	MREGISTER("vxdb.user.get",         m_vxdb_user_get);
	MREGISTER("vxdb.user.remove",      m_vxdb_user_remove);
	MREGISTER("vxdb.user.set",         m_vxdb_user_set);
	MREGISTER("vxdb.vx.bcaps.add",     m_vxdb_vx_bcaps_add);
	MREGISTER("vxdb.vx.bcaps.get",     m_vxdb_vx_bcaps_get);
	MREGISTER("vxdb.vx.bcaps.remove",  m_vxdb_vx_bcaps_remove);
	MREGISTER("vxdb.vx.ccaps.add",     m_vxdb_vx_ccaps_add);
	MREGISTER("vxdb.vx.ccaps.get",     m_vxdb_vx_ccaps_get);
	MREGISTER("vxdb.vx.ccaps.remove",  m_vxdb_vx_ccaps_remove);
	MREGISTER("vxdb.vx.flags.add",     m_vxdb_vx_flags_add);
	MREGISTER("vxdb.vx.flags.get",     m_vxdb_vx_flags_get);
	MREGISTER("vxdb.vx.flags.remove",  m_vxdb_vx_flags_remove);
	MREGISTER("vxdb.vx.limit.get",     m_vxdb_vx_limit_get);
	MREGISTER("vxdb.vx.limit.remove",  m_vxdb_vx_limit_remove);
	MREGISTER("vxdb.vx.limit.set",     m_vxdb_vx_limit_set);
	MREGISTER("vxdb.vx.sched.get",     m_vxdb_vx_sched_get);
	MREGISTER("vxdb.vx.sched.remove",  m_vxdb_vx_sched_remove);
	MREGISTER("vxdb.vx.sched.set",     m_vxdb_vx_sched_set);
	MREGISTER("vxdb.vx.uname.get",     m_vxdb_vx_uname_get);
	MREGISTER("vxdb.vx.uname.remove",  m_vxdb_vx_uname_remove);
	MREGISTER("vxdb.vx.uname.set",     m_vxdb_vx_uname_set);
	MREGISTER("vxdb.xid.get",          m_vxdb_xid_get);
	
	return 0;
}

#undef MREGISTER

xmlrpc_value *method_init(xmlrpc_env *env, xmlrpc_value *p,
                          uint64_t caps, int ownercheck)
{
	char *user, *pass, *name;
	xmlrpc_value *params;
	
	xmlrpc_decompose_value(env, p,
		"({s:s,s:s,*}V)",
		"username", &user,
		"password", &pass,
		&params);
	method_return_if_fault(env);
	
	if (!auth_isvalid(user, pass)) {
		xmlrpc_env_set_fault(env, MEAUTH, method_strerror(MEAUTH));
		return NULL;
	}
	
	if (!auth_capable(user, caps)) {
		xmlrpc_env_set_fault(env, MEPERM, method_strerror(MEPERM));
		return NULL;
	}
	
	if (ownercheck) {
		xmlrpc_decompose_value(env, params, "{s:s,*}", "name", &name);
		method_return_if_fault(env);
		
		if (!auth_isowner(user, name)) {
			xmlrpc_env_set_fault(env, MENOVPS, method_strerror(MENOVPS));
			return NULL;
		}
	}
	
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
