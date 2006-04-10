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

void registry_init(XMLRPC_SERVER s)
{
	/* auth */
	XMLRPC_ServerRegisterMethod(s, "auth.adduser",  m_auth_adduser);
	XMLRPC_ServerRegisterMethod(s, "auth.deluser",  m_auth_deluser);
	XMLRPC_ServerRegisterMethod(s, "auth.getacl",   m_auth_getacl);
	XMLRPC_ServerRegisterMethod(s, "auth.login",  m_auth_login);
	XMLRPC_ServerRegisterMethod(s, "auth.moduser",  m_auth_moduser);
	XMLRPC_ServerRegisterMethod(s, "auth.setacl",   m_auth_setacl);
	XMLRPC_ServerRegisterMethod(s, "auth.userinfo", m_auth_userinfo);
	
	/* vx */
	XMLRPC_ServerRegisterMethod(s, "vx.status", m_vx_status);

	/* vxdb */
	XMLRPC_ServerRegisterMethod(s, "vxdb.get",    m_vxdb_get);
	XMLRPC_ServerRegisterMethod(s, "vxdb.getacl", m_vxdb_getacl);
	XMLRPC_ServerRegisterMethod(s, "vxdb.set",    m_vxdb_set);
	XMLRPC_ServerRegisterMethod(s, "vxdb.setacl", m_vxdb_setacl);
}
