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

#ifndef _VCD_METHODS_H
#define _VCD_METHODS_H

#include "xmlrpc.h"

void registry_init(XMLRPC_SERVER s);

#define MPROTO(NAME) \
	XMLRPC_VALUE NAME (XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)

/* auth */
MPROTO(m_auth_adduser);
MPROTO(m_auth_deluser);
MPROTO(m_auth_getacl);
MPROTO(m_auth_login);
MPROTO(m_auth_moduser);
MPROTO(m_auth_setacl);
MPROTO(m_auth_userinfo);

/* vx */
MPROTO(m_vx_status);

/* vxdb */
MPROTO(m_vxdb_get);
MPROTO(m_vxdb_getacl);
MPROTO(m_vxdb_set);
MPROTO(m_vxdb_setacl);

#undef MPROTO

#endif
