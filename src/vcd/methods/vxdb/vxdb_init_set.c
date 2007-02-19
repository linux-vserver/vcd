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
#include "validate.h"
#include "vxdb.h"

#include <lucid/log.h>
#include <lucid/str.h>

xmlrpc_value *m_vxdb_init_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name, *init, *halt, *reboot;
	int rc, timeout = 0;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_INIT, M_OWNER|M_LOCK);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,s:s,s:s,s:i,*}",
			"name", &name,
			"init", &init,
			"halt", &halt,
			"reboot", &reboot,
			"timeout", &timeout);
	method_return_if_fault(env);

	method_empty_params(3, &init, &halt, &reboot);

	if (!init)
		init = "/sbin/init";

	if (!halt)
		halt = "/sbin/halt";

	if (!reboot)
		reboot = "/sbin/reboot";

	if (!str_path_isabs(init))
		method_return_faultf(env, MEINVAL,
				"invalid init value: %s", init);

	if (!str_path_isabs(halt))
		method_return_faultf(env, MEINVAL,
				"invalid halt value: %s", halt);

	if (!str_path_isabs(reboot))
		method_return_faultf(env, MEINVAL,
				"invalid reboot value: %s", reboot);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	rc = vxdb_exec(
			"INSERT OR REPLACE INTO init (xid, init, halt, reboot, timeout) "
			"VALUES (%d, '%s', '%s', '%s', %d)",
			xid, init, halt, reboot, timeout);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	return xmlrpc_nil_new(env);
}
