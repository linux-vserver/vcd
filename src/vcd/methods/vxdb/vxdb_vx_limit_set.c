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
#include <inttypes.h>

#include "auth.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

#include <lucid/log.h>
#include <lucid/scanf.h>
#include <lucid/str.h>

static
uint64_t bytestopages(uint64_t bytes) {
	uint64_t pages;
	pages = (bytes * 1024) / (uint64_t) getpagesize();
	return pages;
}

static
struct limtype_data {
	char *type;
} LIMTYPE[] = {
	{ "RLIMIT_AS"      },
	{ "RLIMIT_MEMLOCK" },
	{ "RLIMIT_RSS"     },
	{ "VLIMIT_ANON"    },
	{ "VLIMIT_MAPPED"  },
	{ "VLIMIT_SHMEM"   },
	{ NULL             }
};

/* vxdb.vx.limit.set(string name, string type, string soft, string max) */
xmlrpc_value *m_vxdb_vx_limit_set(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *name, *type, *softp, *maxp;
	xid_t xid;
	int i, rc;

	params = method_init(env, p, c, VCD_CAP_RLIM, M_OWNER|M_LOCK);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,s:s,s:s,*}",
			"name", &name,
			"type", &type,
			"soft", &softp,
			"max", &maxp);
	method_return_if_fault(env);

	if (!str_isdigit(softp))
		method_return_faultf(env, MEINVAL,
				"soft value is not a number: %s", softp);

	if (!str_isdigit(maxp))
		method_return_faultf(env, MEINVAL,
				"max value is not a number: %s", maxp);

	uint64_t soft = CRLIM_KEEP;
	uint64_t max  = CRLIM_KEEP;

	sscanf(softp, "%" SCNu64, &soft);
	sscanf(maxp,  "%" SCNu64, &max);

	if (!validate_rlimit(type))
		method_return_faultf(env, MEINVAL,
				"invalid type value: %s", type);

	if (!validate_rlimits(soft, max))
		method_return_faultf(env, MEINVAL,
				"invalid limit specification: %" PRIu64 ",%" PRIu64,
				soft, max);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	for(i = 0; LIMTYPE[i].type; i++) {
		if (str_equal(type, LIMTYPE[i].type)) {
			soft = bytestopages(soft);
			max = bytestopages(max);
		}
	}

	rc = vxdb_exec(
			"INSERT OR REPLACE INTO vx_limit (xid, type, soft, max) "
			"VALUES (%d, '%s', '%" PRIu64 "', '%" PRIu64 "')",
			xid, type, soft, max);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	return xmlrpc_nil_new(env);
}
