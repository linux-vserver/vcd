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

#ifndef _METHODS_AUTH_H
#define _METHODS_AUTH_H

#include <stdint.h>

#include "lucid.h"
#include "xmlrpc.h"

/* capabilities */
#define VCD_CAP_VXDB_GET 0x00000001
#define VCD_CAP_VXDB_SET 0x00000002
#define VCD_CAP_ADMIN    0x80000000

extern const flist64_t vcd_caps_list[];

int auth_isvalid(XMLRPC_VALUE auth);
int auth_capable(XMLRPC_VALUE auth, uint64_t cap);
int auth_vxowner(XMLRPC_VALUE auth, char *name);
int auth_isuser (XMLRPC_VALUE auth, char *user);

#endif
