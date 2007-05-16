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

#include <unistd.h>

#include "auth.h"
#include "methods.h"
#include "vxdb.h"

#include <lucid/log.h>
#include <lucid/str.h>

/* vx.stop(string name) */
xmlrpc_value *m_vx_stop(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name, *halt = "/sbin/halt";
	int rc, timeout = 15;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_INIT, M_OWNER|M_LOCK);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,*}",
			"name", &name);
	method_return_if_fault(env);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	if (vx_info(xid, NULL) == -1) {
		if (errno == ESRCH)
			method_return_fault(env, MESTOPPED);
		else
			method_return_sys_fault(env, "vx_info");
	}

	rc = vxdb_prepare(&dbr,
			"SELECT halt,timeout FROM init WHERE xid = %d", xid);

	if (rc == VXDB_OK) {
		vxdb_foreach_step(rc, dbr) {
			halt = str_dup(vxdb_column_text(dbr, 0));
			timeout = vxdb_column_int(dbr, 1);
			timeout = timeout < 1 ? 15 : timeout;
		}
	}

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);
	method_return_if_fault(env);

	params = xmlrpc_build_value(env,
			"{s:s,s:s}",
			"name",    name,
			"command", halt);

	m_vx_exec(env, params, METHOD_INTERNAL);
	method_return_if_fault(env);

	while (timeout-- > 0) {
		if (vx_info(xid, NULL) == -1) {
			if (errno == ESRCH)
				return xmlrpc_nil_new(env);
			else
				method_return_sys_fault(env, "vx_info");
		}

		sleep(1);
	}

	method_return_faultf(env, MEBUSY,
			"vx still alive after timeout (%d)", timeout);
}
