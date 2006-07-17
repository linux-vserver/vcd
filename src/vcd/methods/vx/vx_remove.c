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

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <vserver.h>
#include <sys/stat.h>

#include "lucid.h"

#include "auth.h"
#include "cfg.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

/* vx.remove(string name) */
xmlrpc_value *m_vx_remove(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	char *name, vdir[PATH_MAX];
	xid_t xid;
	int rc;
	const char *vserverdir;
	struct stat sb;
	
	params = method_init(env, p, VCD_CAP_CREATE, M_OWNER|M_LOCK);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,*}",
		"name", &name);
	method_return_if_fault(env);
	
	if (!validate_name(name))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	if (vx_get_info(xid, NULL) != -1)
		method_return_fault(env, MERUNNING);
	
	rc = vxdb_exec("BEGIN TRANSACTION;"
		"DELETE FROM dx_limit     WHERE xid = %1$d;"
		"DELETE FROM init_method  WHERE xid = %1$d;"
		"DELETE FROM mount        WHERE xid = %1$d;"
		"DELETE FROM nx_addr      WHERE xid = %1$d;"
		"DELETE FROM nx_broadcast WHERE xid = %1$d;"
		"DELETE FROM vx_bcaps     WHERE xid = %1$d;"
		"DELETE FROM vx_ccaps     WHERE xid = %1$d;"
		"DELETE FROM vx_flags     WHERE xid = %1$d;"
		"DELETE FROM vx_limit     WHERE xid = %1$d;"
		"DELETE FROM vx_sched     WHERE xid = %1$d;"
		"DELETE FROM vx_uname     WHERE xid = %1$d;"
		"DELETE FROM xid_name_map WHERE xid = %1$d;"
		"DELETE FROM xid_uid_map  WHERE xid = %1$d;"
		"COMMIT TRANSACTION;",
		xid);
	
	if (rc)
		method_return_fault(env, MEVXDB);
	
	vserverdir = cfg_getstr(cfg, "vserverdir");
	
	snprintf(vdir, PATH_MAX, "%s/%s", vserverdir, name);
	
	if (runlink(vdir) == -1)
		method_return_faultf(env, MESYS, "runlink: %s", strerror(errno));
	
	if (lstat(vdir, &sb) == -1) {
		if (errno != ENOENT)
			method_return_faultf(env, MESYS, "lstat: %s", strerror(errno));
	}
	
	else
		method_return_faultf(env, MEEXIST, "vdir still exists after runlink: %s", vdir);
	
	return xmlrpc_nil_new(env);
}
