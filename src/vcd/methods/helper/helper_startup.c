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
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <vserver.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <lucid/bitmap.h>
#include <lucid/chroot.h>
#include <lucid/exec.h>
#include <lucid/str.h>

#include "auth.h"
#include "cfg.h"
#include "lists.h"
#include "log.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

#ifndef MS_REC
#define MS_REC 16384
#endif

/* startup process:
   1) setup capabilities
   2) setup resource limits
   3) setup scheduler
   4) setup unames
   5) setup filesystem namespace
*/

static const char *name = NULL;
static xid_t xid = 0;

static
xmlrpc_value *context_caps_and_flags(xmlrpc_env *env)
{
	int rc;
	vxdb_result *dbr;
	
	struct vx_bcaps bcaps = {
		.bcaps = 0,
		.bmask = 0,
	};
	
	struct vx_ccaps ccaps = {
		.ccaps = 0,
		.cmask = 0,
	};
	
	struct vx_flags cflags = {
		.flags = 0,
		.mask  = 0,
	};
	
	/* 1.2.1) setup system capabilities */
	rc = vxdb_prepare(&dbr, "SELECT bcap FROM vx_bcaps WHERE xid = %d", xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else
		vxdb_foreach_step(rc, dbr)
			bcaps.bmask |= flist64_getval(bcaps_list, sqlite3_column_text(dbr, 0));
	
	if (rc == -1)
		method_set_fault(env, MEVXDB);
	
	sqlite3_finalize(dbr);
	method_return_if_fault(env);
	
	if (vx_set_bcaps(xid, &bcaps) == -1)
		method_return_faultf(env, MESYS, "vx_set_bcaps: %s", strerror(errno));
	
	/* 1.2.2) setup context capabilities */
	rc = vxdb_prepare(&dbr, "SELECT ccap FROM vx_ccaps WHERE xid = %d", xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else
		vxdb_foreach_step(rc, dbr)
			ccaps.ccaps |= flist64_getval(ccaps_list, sqlite3_column_text(dbr, 0));
	
	ccaps.cmask = ccaps.ccaps;
	
	if (rc == -1)
		method_set_fault(env, MEVXDB);
	
	sqlite3_finalize(dbr);
	method_return_if_fault(env);
	
	if (vx_set_ccaps(xid, &ccaps) == -1)
		method_return_faultf(env, MESYS, "vx_set_ccaps: %s", strerror(errno));
	
	/* 1.2.3) setup context flags */
	rc = vxdb_prepare(&dbr, "SELECT flag FROM vx_flags WHERE xid = %d", xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else
		vxdb_foreach_step(rc, dbr)
			cflags.flags |= flist64_getval(cflags_list, sqlite3_column_text(dbr, 0));
	
	cflags.mask = cflags.flags;
	
	if (rc == -1)
		method_set_fault(env, MEVXDB);
	
	sqlite3_finalize(dbr);
	method_return_if_fault(env);
	
	if (vx_set_flags(xid, &cflags) == -1)
		method_return_faultf(env, MESYS, "vx_set_flags: %s", strerror(errno));
	
	return NULL;
}

static
xmlrpc_value *context_resource_limits(xmlrpc_env *env)
{
	vxdb_result *dbr;
	int rc;
	const char *type;
	uint32_t buf32;
	
	struct vx_rlimit_mask rlimit_mask = {
		.minimum   = 0,
		.softlimit = 0,
		.maximum   = 0,
	};
	
	struct vx_rlimit rlimit = {
		.id        = -1,
		.minimum   = 0,
		.softlimit = 0,
		.maximum   = 0,
	};
	
	if (vx_get_rlimit_mask(&rlimit_mask) == -1)
		method_return_faultf(env, MESYS, "vx_get_rlimit_mask: %s", strerror(errno));
	
	rc = vxdb_prepare(&dbr,
		"SELECT type,soft,max FROM vx_limit WHERE xid = %d",
		xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else {
		vxdb_foreach_step(rc, dbr) {
			type = sqlite3_column_text(dbr, 0);
			
			if (!(buf32 = flist32_getval(rlimit_list, type)))
				continue;
			
			if ((rlimit_mask.softlimit & buf32) != buf32 &&
					(rlimit_mask.maximum   & buf32) != buf32)
				continue;
			
			rlimit.id        = v2i32(buf32);
			rlimit.softlimit = sqlite3_column_int64(dbr, 1);
			rlimit.maximum   = sqlite3_column_int64(dbr, 2);
			
			if (rlimit.softlimit == 0)
				rlimit.softlimit = CRLIM_INFINITY;
			
			if (rlimit.maximum == 0)
				rlimit.maximum = CRLIM_INFINITY;
			
			if (rlimit.maximum < rlimit.softlimit)
				rlimit.maximum = rlimit.softlimit;
			
			if (vx_set_rlimit(xid, &rlimit) == -1) {
				method_set_faultf(env, MESYS, "vx_set_rlimit: %s", strerror(errno));
				break;
			}
		}
		
		if (rc == -1)
			method_set_fault(env, MEVXDB);
	}
	
	sqlite3_finalize(dbr);
	
	return NULL;
}

static
xmlrpc_value *context_scheduler(xmlrpc_env *env)
{
	int rc;
	vxdb_result *dbr;
	
	struct vx_sched sched = {
		.set_mask = 0,
		.fill_rate = 0,
		.interval = 0,
		.tokens = 0,
		.tokens_min = 0,
		.tokens_max = 0,
		.prio_bias = 0,
		.cpu_id = 0,
		.bucket_id = 0,
	};
	
	rc = vxdb_prepare(&dbr,
		"SELECT cpuid,fillrate,interval,priobias,tokensmin,tokensmax,fillrate2,interval2 "
		"FROM vx_sched WHERE xid = %d",
		xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else {
		vxdb_foreach_step(rc, dbr) {
			sched.cpu_id     = sqlite3_column_int(dbr, 0);
			sched.fill_rate  = sqlite3_column_int(dbr, 1);
			sched.interval   = sqlite3_column_int(dbr, 2);
			sched.prio_bias  = sqlite3_column_int(dbr, 3);
			sched.tokens_min = sqlite3_column_int(dbr, 4);
			sched.tokens_max = sqlite3_column_int(dbr, 5);
			
			sched.tokens = sched.tokens_max;
			
			sched.set_mask |= VXSM_CPU_ID|VXSM_FILL_RATE|VXSM_INTERVAL|VXSM_TOKENS;
			sched.set_mask |= VXSM_TOKENS_MIN|VXSM_TOKENS_MAX|VXSM_PRIO_BIAS;
			
			if (vx_set_sched(xid, &sched) == -1) {
				method_set_faultf(env, MESYS, "vx_set_sched: %s", strerror(errno));
				break;
			}
			
			sched.fill_rate  = sqlite3_column_int(dbr, 6);
			sched.interval   = sqlite3_column_int(dbr, 7);
			
			sched.set_mask = 0|VXSM_FILL_RATE2|VXSM_INTERVAL2;
			
			if (sched.fill_rate > 0 && sched.interval > 0 &&
			    vx_set_sched(xid, &sched) == -1) {
				method_set_faultf(env, MESYS, "vx_set_sched2: %s", strerror(errno));
				break;
			}
		}
		
		if (rc == -1)
			method_set_fault(env, MEVXDB);
	}
	
	sqlite3_finalize(dbr);
	return NULL;
}

static
xmlrpc_value *context_uname(xmlrpc_env *env)
{
	int rc;
	vxdb_result *dbr;
	const char *uname;
	uint32_t buf32;
	
	struct vx_vhi_name vhiname = {
		.field = 0,
		.name  = "",
	};
	
	rc = vxdb_prepare(&dbr,
		"SELECT uname,value FROM vx_uname WHERE xid = %d",
		xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else {
		vxdb_foreach_step(rc, dbr) {
			uname = sqlite3_column_text(dbr, 0);
			
			if (!(buf32 = flist32_getval(vhiname_list, uname)))
				continue;
			
			vhiname.field = v2i32(buf32);
			
			bzero(vhiname.name, VHILEN);
			strncpy(vhiname.name, sqlite3_column_text(dbr, 1), VHILEN-1);
			
			if (vx_set_vhi_name(xid, &vhiname) == -1) {
				method_set_faultf(env, MESYS, "vx_set_vhi_name: %s", strerror(errno));
				break;
			}
		}
		
		if (rc == -1)
			method_set_fault(env, MEVXDB);
	}
	
	sqlite3_finalize(dbr);
	method_return_if_fault(env);
	
	if (!str_isempty(name)) {
		vhiname.field = VHIN_CONTEXT;
		
		bzero(vhiname.name, VHILEN);
		strncpy(vhiname.name, name, VHILEN-1);
		
		if (vx_set_vhi_name(xid, &vhiname) == -1)
			method_set_faultf(env, MESYS, "vx_set_vhi_name: %s", strerror(errno));
	}
	
	return NULL;
}

static
xmlrpc_value *namespace_setup(xmlrpc_env *env, const char *vdir)
{
	pid_t pid;
	int status;
	
	switch ((pid = vx_clone_namespace(SIGCHLD, NULL))) {
	case -1:
		method_return_faultf(env, MESYS, "vx_clone_namespace: %s", strerror(errno));
	
	case 0:
		if (chroot_secure_chdir(vdir, "/") == -1)
			log_error_and_die("chroot_secure_chdir(%s): %s", vdir, strerror(errno));
		
		if (mount(".", "/", NULL, MS_BIND|MS_REC, NULL) == -1)
			log_error_and_die("mount: %s", strerror(errno));
		
		if (vx_set_namespace(xid) == -1)
			log_error_and_die("vx_set_namespace: %s", strerror(errno));
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			method_return_faultf(env, MESYS, "%s: waitpid: %s", __FUNCTION__, strerror(errno));
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			method_return_faultf(env, MESYS, "%s", "namespace_setup failed");
	}
	
	return NULL;
}

/* helper.startup(int xid) */
xmlrpc_value *m_helper_startup(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	const char *init = "/sbin/init", *vserverdir;
	char vdir[PATH_MAX];
	vxdb_result *dbr;
	int rc;
	
	params = method_init(env, p, c, VCD_CAP_HELPER, 0);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:i,*}",
		"xid", &xid);
	method_return_if_fault(env);
	
	if (!(name = vxdb_getname(xid)))
		method_return_fault(env, MENOVPS);
	
	if (vx_get_info(xid, NULL) == -1)
		method_return_fault(env, MESTOPPED);
	
	vserverdir = cfg_getstr(cfg, "vserverdir");
	snprintf(vdir, PATH_MAX, "%s/%s", vserverdir, name);
	
	rc = vxdb_prepare(&dbr, "SELECT init FROM init WHERE xid = %d", xid);
	
	if (rc)
		method_set_fault(env, MEVXDB);
	
	else {
		rc = vxdb_step(dbr);
		
		if (rc == -1)
			method_set_fault(env, MEVXDB);
		
		else if (rc > 0)
			init = strdup(sqlite3_column_text(dbr, 0));
	}
	
	sqlite3_finalize(dbr);
	method_return_if_fault(env);
	
	context_caps_and_flags(env);
	method_return_if_fault(env);
	
	context_resource_limits(env);
	method_return_if_fault(env);
	
	context_scheduler(env);
	method_return_if_fault(env);
	
	context_uname(env);
	method_return_if_fault(env);
	
	namespace_setup(env, vdir);
	method_return_if_fault(env);
	
	return xmlrpc_build_value(env, "{s:s,s:s}", "vdir", vdir, "init", init);
}
