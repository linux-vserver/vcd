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

#include "xmlrpc.h"

#include "methods.h"

#define MREGISTER(NAME,FUNC)  XMLRPC_ServerRegisterMethod(s, NAME, FUNC)

void registry_init(XMLRPC_SERVER s)
{
	/* vxdb */
/*
	MREGISTER("vxdb.dx.limit.get", m_vxdb_dx_limit_get);
	MREGISTER("vxdb.dx.limit.set", m_vxdb_dx_limit_set);
	
	MREGISTER("vxdb.init.method.get", m_vxdb_init_method_get);
	MREGISTER("vxdb.init.method.set", m_vxdb_init_method_set);
	
	MREGISTER("vxdb.init.mount.add", m_vxdb_init_mount_add);
	MREGISTER("vxdb.init.mount.get", m_vxdb_init_mount_get);
	MREGISTER("vxdb.init.mount.remove", m_vxdb_init_mount_remove);
	
	MREGISTER("vxdb.nx.addr.add", m_vxdb_nx_addr_add);
	MREGISTER("vxdb.nx.addr.get", m_vxdb_nx_addr_get);
	MREGISTER("vxdb.nx.addr.remove", m_vxdb_nx_add_remove);
	
	MREGISTER("vxdb.owner.add", m_vxdb_owner_add);
	MREGISTER("vxdb.owner.get", m_vxdb_owner_get);
	MREGISTER("vxdb.owner.remove", m_vxdb_owner_remove);
	
	MREGISTER("vxdb.user.add", m_vxdb_user_add);
	MREGISTER("vxdb.user.get", m_vxdb_user_get);
	MREGISTER("vxdb.user.set", m_vxdb_user_set);
	MREGISTER("vxdb.user.remove", m_vxdb_user_remove);
	
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
	MREGISTER("vxdb.vx.limit.set", m_vxdb_vx_limit_set);
	
	MREGISTER("vxdb.vx.pflags.add", m_vxdb_vx_pflags_add);
	MREGISTER("vxdb.vx.pflags.get", m_vxdb_vx_pflags_get);
	MREGISTER("vxdb.vx.pflags.remove", m_vxdb_vx_pflags_remove);
	
	MREGISTER("vxdb.vx.sched.get", m_vxdb_vx_sched_get);
	MREGISTER("vxdb.vx.sched.set", m_vxdb_vx_sched_set);
	
	MREGISTER("vxdb.vx.uname.get", m_vxdb_vx_uname_get);
	MREGISTER("vxdb.vx.uname.set", m_vxdb_vx_uname_set);
*/
	
	MREGISTER("vxdb.create", m_vxdb_create);
	MREGISTER("vxdb.remove", m_vxdb_remove);
}

#undef MREGISTER

XMLRPC_VALUE get_method_params(XMLRPC_REQUEST r)
{
	XMLRPC_VALUE request, auth, params;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	params  = XMLRPC_VectorNext(request);
	
	return params;
}
