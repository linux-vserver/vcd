// Copyright 2007 Luca Longinotti <chtekk@gentoo.org>
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
#include "validate.h"
#include "vxdb.h"

#include <lucid/list.h>
#include <lucid/log.h>
#include <lucid/mem.h>
#include <lucid/str.h>

/* vg.vx.start(string groupname) */
xmlrpc_value *m_vg_vx_start(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	typedef struct {
		list_t list;
		xid_t  xid;
		const char *name;
	} xn_list_t;

	xmlrpc_value *params;
	char *group;
	int rc, gid;
	xn_list_t _xns, *xns = &_xns, *pos;

	params = method_init(env, p, c, VCD_CAP_INIT, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,*}",
			"group", &group);
	method_return_if_fault(env);

	if (!validate_group(group))
		method_return_faultf(env, MEINVAL,
				"invalid group value: %s", group);

	if (str_equal(group, "all"))
		rc = vxdb_prepare(&dbr,
				"SELECT xid,name FROM xid_name_map");

	else {
		if (!(gid = vxdb_getgid(group)))
			method_return_fault(env, MENOVG);

		rc = vxdb_prepare(&dbr,
			"SELECT xid_name_map.xid,xid_name_map.name FROM xid_name_map "
			"INNER JOIN xid_gid_map "
			"ON xid_name_map.xid = xid_gid_map.xid "
			"WHERE xid_gid_map.gid = %d",
			gid);
	}

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	INIT_LIST_HEAD(&(xns->list));

	vxdb_foreach_step(rc, dbr) {
		LIST_NODE_ALLOC(xn_list_t, new);

		new->xid  = vxdb_column_int(dbr, 0);
		new->name = str_dup(vxdb_column_text(dbr, 1));

		list_add(&(new->list), &(xns->list));
	}

	if (rc != VXDB_DONE)
		method_return_vxdb_fault(env);

	vxdb_finalize(dbr);

	list_for_each_entry(pos, &(xns->list), list) {
		if (vx_info(pos->xid, NULL) == -1) {
			if (errno == ESRCH) {
				params = xmlrpc_build_value(env, "{s:s}", "name", pos->name);
				m_vx_start(env, params, METHOD_INTERNAL);
			}

			else
				method_return_sys_fault(env, "vx_info");
		}
	}

	return xmlrpc_nil_new(env);
}
