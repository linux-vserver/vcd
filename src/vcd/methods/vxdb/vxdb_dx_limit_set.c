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

#include <inttypes.h>

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

#include <lucid/log.h>
#include <lucid/scanf.h>
#include <lucid/str.h>

xmlrpc_value *m_vxdb_dx_limit_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name, *path, *spacep, *inodesp;
	int reserved, rc;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_DLIM, M_OWNER|M_LOCK);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,s:s,s:s,s:i,*}",
			"name", &name,
			"path", &path,
			"space", &spacep,
			"inodes", &inodesp,
			"reserved", &reserved);
	method_return_if_fault(env);

	if (!str_isdigit(spacep))
		method_return_faultf(env, MEINVAL,
				"space value is not a number: %s", spacep);

	if (!str_isdigit(inodesp))
		method_return_faultf(env, MEINVAL,
				"inodes value is not a number: %s", inodesp);

	uint32_t space  = CDLIM_KEEP;
	uint32_t inodes = CDLIM_KEEP;

	sscanf(spacep,  "%" SCNu32, &space);
	sscanf(inodesp, "%" SCNu32, &inodes);

	if (!validate_path(path))
		method_return_faultf(env, MEINVAL,
				"invalid path value: %s", path);
	
	if (!validate_dlimits(space, inodes, reserved))
		method_return_faultf(env, MEINVAL,
				"invalid limit specification: %" PRIu32 ",%" PRIu32 ",%d",
				space, inodes, reserved);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	rc = vxdb_exec(
			"INSERT OR REPLACE INTO dx_limit "
			"(xid, path, space, inodes, reserved) "
			"VALUES (%d, '%s', '%" PRIu32 "', '%" PRIu32 "', %d)",
			xid, path, space, inodes, reserved);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	return xmlrpc_nil_new(env);
}
