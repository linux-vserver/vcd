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

xmlrpc_value *m_vg_vx_list(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	typedef struct {
		list_t list;
		xid_t   xid;
	} xid_list_t;

	xmlrpc_value *params, *response = NULL;
	char *group, *vsname;
	int rc, gid;
	xid_list_t _xids, *xids = &_xids, *st;
	list_t *pos;

	params = method_init(env, p, c, VCD_CAP_AUTH, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,*}",
			"groupname", &group);
	method_return_if_fault(env);

	if (!validate_groupname(group))
		method_return_faultf(env, MEINVAL,
				"invalid groupname value: %s", group);

	if (str_equal(group, "default")) {
		rc = vxdb_prepare(&dbr,
				"SELECT xid FROM xid_name_map ORDER BY xid DESC");

		if (rc != VXDB_OK)
			method_return_vxdb_fault(env);
	}

	else {
		gid = vxdb_getgid(group);

		if (gid != 0) {
			rc = vxdb_prepare(&dbr,
					"SELECT xid FROM xid_gid_map WHERE gid = %d ORDER BY xid DESC",
					gid);

			if (rc != VXDB_OK)
				method_return_vxdb_fault(env);
		}

		else
			method_return_faultf(env, MEINVAL,
					"group doesn't exist: %s", group);
	}

	INIT_LIST_HEAD(&(xids->list));

	vxdb_foreach_step(rc, dbr) {
		xid_list_t *new = mem_alloc(sizeof(xid_list_t));
		new->xid = vxdb_column_int(dbr, 0);
		list_add(&(new->list), &(xids->list));
	}

	 if (rc != VXDB_DONE)
		method_return_vxdb_fault(env);

	vxdb_finalize(dbr);

	response = xmlrpc_array_new(env);

	list_for_each(pos, &(xids->list)) {
		st = list_entry(pos, xid_list_t, list);

		vsname = vxdb_getname(st->xid);

		xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:i}",
				"vsname", vsname,
				"xid",    st->xid));
	}

	return response;
}
