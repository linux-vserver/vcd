// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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

#include "auth.h"
#include "methods.h"
#include "vxdb.h"

#include <lucid/log.h>

/* vxdb.name.get(int xid) */
xmlrpc_value *m_vxdb_name_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_INFO, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:i,*}",
			"xid", &xid);
	method_return_if_fault(env);

	if (!(name = vxdb_getname(xid)))
		method_return_fault(env, MENOVPS);

	return xmlrpc_build_value(env, "s", name);
}
