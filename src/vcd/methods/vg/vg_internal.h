// Copyright 2007 Luca Longinotti <chtekk@gentoo.org>
//           2007 Benedikt Böhm <hollow@gentoo.org>
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

#ifndef _VCD_VG_INTERNAL_H
#define _VCD_VG_INTERNAL_H

#include <vserver.h>
#include <lucid/list.h>

#include "methods.h"

typedef struct {
	list_t list;
	xid_t  xid;
	char *name;
} vg_list_t;

xmlrpc_value *vg_list_init(xmlrpc_env *env, const char *group, vg_list_t *vxs);
void vg_list_free(vg_list_t *vxs);

#endif
