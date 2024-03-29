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
#include <lucid/printf.h>
#include <lucid/str.h>

static
uint64_t pagestobytes(uint64_t pages) {
	uint64_t bytes;
	bytes = (pages * (uint64_t) getpagesize()) / 1024;
	return bytes;
}

static
struct limtype_data {
	char *type;
} LIMTYPE[] = {
	{ "ANON"    },
	{ "AS"      },
	{ "MAPPED"  },
	{ "MEMLOCK" },
	{ "RSS"     },
	{ "SHMEM"   },
	{ NULL      }
};

/* vxdb.vx.limit.get(string name[, string type]) */
xmlrpc_value *m_vxdb_vx_limit_get(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params, *response = NULL;
	char *name, *type;
	int i, rc;
	xid_t xid;

	params = method_init(env, p, c, VCD_CAP_RLIM, M_OWNER);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,*}",
			"name", &name,
			"type", &type);
	method_return_if_fault(env);

	method_empty_params(1, &type);

	if (type && !validate_rlimit(type))
		method_return_faultf(env, MEINVAL,
				"invalid type value: %s", type);

	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);

	if (type)
		rc = vxdb_prepare(&dbr,
				"SELECT type,soft,max FROM vx_limit "
				"WHERE xid = %d AND type = '%s'",
				xid, type);

	else
		rc = vxdb_prepare(&dbr,
				"SELECT type,soft,max FROM vx_limit "
				"WHERE xid = %d",
				xid);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	response = xmlrpc_array_new(env);

	vxdb_foreach_step(rc, dbr) {
		const char *typedb = vxdb_column_text(dbr, 0);
		uint64_t soft = vxdb_column_uint64(dbr, 1);
		uint64_t max  = vxdb_column_uint64(dbr, 2);

		for(i = 0; LIMTYPE[i].type; i++) {
			if (str_equal(typedb, LIMTYPE[i].type)) {
				soft = pagestobytes(soft);
				max  = pagestobytes(max);
			}
		}

		char *softs, *maxs;
		asprintf(&softs, "%" PRIu64, soft);
		asprintf(&maxs,  "%" PRIu64, max);

		xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
				"{s:s,s:s,s:s}",
				"type", typedb,
				"soft", softs,
				"max",  maxs));
	}

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);

	return response;
}
