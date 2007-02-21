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

#include "auth.h"
#include "methods.h"

#include <lucid/list.h>
#include <lucid/log.h>

#include "vg_internal.h"

/* vg.vx.start(string groupname) */
xmlrpc_value *m_vg_vx_start(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *group;
	vg_list_t _vxs, *vxs = &_vxs, *pos;

	params = method_init(env, p, c, VCD_CAP_INIT, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,*}",
			"group", &group);
	method_return_if_fault(env);

	/* get group list */
	vg_list_init(env, group, vxs);
	method_return_if_fault(env);

	/* iterate over vxs and start not running guests */
	list_for_each_entry(pos, &(vxs->list), list) {
		if (vx_info(pos->xid, NULL) == -1) {
			if (errno == ESRCH) {
				params = xmlrpc_build_value(env, "{s:s}", "name", pos->name);
				m_vx_start(env, params, METHOD_INTERNAL);
			}

			else
				method_return_sys_fault(env, "vx_info");
		}
	}

	vg_list_free(vxs);

	return xmlrpc_nil_new(env);
}
