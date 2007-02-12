// Copyright 2007 Benedikt Böhm <hollow@gentoo.org>
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

/* vcd.status() */
xmlrpc_value *m_vcd_status(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	method_init(env, p, c, VCD_CAP_INFO, 0);
	method_return_if_fault(env);

	int rc = vxdb_prepare(&dbr,
			"SELECT uptime, requests, flogins, nomethod FROM vcd "
			"ORDER BY uptime DESC LIMIT 1");

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	rc = vxdb_step(dbr);

	xmlrpc_value *response = NULL;

	if (rc == VXDB_ROW)
		response = xmlrpc_build_value(env, "{s:i,s:i,s:i,s:i}",
				"uptime", time(NULL) - vxdb_column_int(dbr, 0),
				"requests", vxdb_column_int(dbr, 1),
				"nomethod", vxdb_column_int(dbr, 2),
				"flogins", vxdb_column_int(dbr, 3));
	else if (rc == VXDB_DONE)
		response = xmlrpc_nil_new(env);
	else
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);

	return response;
}
