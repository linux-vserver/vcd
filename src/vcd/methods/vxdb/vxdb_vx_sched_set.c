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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xmlrpc.h"

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vxdb.vx.sched.set(string name, int fillrate, int interval,
                  int tokensmin, int tokensmax[,
                  int fillrate2[, int interval2[,
                  int priobias[, int cpuid]]]])
*/
XMLRPC_VALUE m_vxdb_vx_sched_set(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	char *name  = XMLRPC_VectorGetStringWithID(params, "name");
	
	int fillrate  = XMLRPC_VectorGetIntWithID(params, "fillrate");
	int interval  = XMLRPC_VectorGetIntWithID(params, "interval");
	int fillrate2 = XMLRPC_VectorGetIntWithID(params, "fillrate2");
	int interval2 = XMLRPC_VectorGetIntWithID(params, "interval2");
	int tokensmin = XMLRPC_VectorGetIntWithID(params, "tokensmin");
	int tokensmax = XMLRPC_VectorGetIntWithID(params, "tokensmax");
	int priobias  = XMLRPC_VectorGetIntWithID(params, "priobias");
	int cpuid     = XMLRPC_VectorGetIntWithID(params, "cpuid");
	
	if (!validate_name(name) || !validate_cpuid(cpuid))
		return method_error(MEREQ);
	
	if (!validate_token_bucket(fillrate, interval,
	                           fillrate2, interval2,
	                           tokensmin, tokensmax, priobias))
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	dbr = dbi_conn_queryf(vxdb,
		"INSERT OR REPLACE INTO vx_sched (xid, fill_rate, interval, fill_rate2, "
		"interval2, tokens_min, tokens_max, prio_bias, cpu_id) "
		"VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d)",
		xid, fillrate, interval, fillrate2, interval2,
		tokensmin, tokensmax, priobias, cpuid);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	return NULL;
}
