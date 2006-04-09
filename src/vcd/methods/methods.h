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

/* auth */
XMLRPC_VALUE m_auth_adduser(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d);
XMLRPC_VALUE m_auth_deluser(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d);
XMLRPC_VALUE m_auth_getacl (XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d);
XMLRPC_VALUE m_auth_moduser(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d);
XMLRPC_VALUE m_auth_setacl (XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d);

/* vx */
XMLRPC_VALUE m_vx_status(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d);

/* vxdb */
XMLRPC_VALUE m_vxdb_get(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d);
XMLRPC_VALUE m_vxdb_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d);

#endif
