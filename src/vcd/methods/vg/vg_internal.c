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

#include <lucid/mem.h>
#include <lucid/str.h>

#include "validate.h"
#include "vxdb.h"

#include "vg_internal.h"

xmlrpc_value *vg_list_init(xmlrpc_env *env, const char *group, vg_list_t *vxs)
{
	int rc, gid = 0;
	vg_list_t *new;

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

	INIT_LIST_HEAD(&(vxs->list));

	vxdb_foreach_step(rc, dbr) {
		LIST_NODE_ALLOC(new);

		new->xid  = vxdb_column_int(dbr, 0);
		new->name = str_dup(vxdb_column_text(dbr, 1));

		list_add(&(new->list), &(vxs->list));
	}

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);

	return NULL;
}

void vg_list_free(vg_list_t *vxs)
{
	vg_list_t *p, *tmp;

	list_for_each_entry_safe(p, tmp, &(vxs->list), list) {
		list_del(&(p->list));
		mem_free(p->name);
		mem_free(p);
	}
}
