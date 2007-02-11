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
#include "stats.h"

#include <lucid/log.h>

/* vcd.status() */
xmlrpc_value *m_vcd_status(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	method_init(env, p, c, VCD_CAP_INFO, 0);
	method_return_if_fault(env);

	return xmlrpc_build_value(env, "{s:i,s:i,s:i,s:i,s:i}",
			"uptime", vcd_stats->uptime,
			"requests", vcd_stats->requests,
			"nosuchmethod", vcd_stats->nosuchmethod,
			"failedlogins", vcd_stats->failedlogins,
			"vxdbqueries", vcd_stats->vxdbqueries);
}
