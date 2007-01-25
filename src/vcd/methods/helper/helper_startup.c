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
#include <lucid/log.h>
#include <lucid/open.h>
#include <lucid/str.h>

#include "auth.h"
#include "cfg.h"
#include "lists.h"
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
	vx_flags_t bcaps, ccaps, cflags;

	/* 1.1) setup system capabilities */
	rc = vxdb_prepare(&dbr, "SELECT bcap FROM vx_bcaps WHERE xid = %d", xid);

	if (rc)
		method_set_fault(env, MEVXDB);

	else
		vxdb_foreach_step(rc, dbr)
			bcaps.mask |= flist64_getval(bcaps_list, sqlite3_column_text(dbr, 0));

	if (rc == -1)
		method_set_fault(env, MEVXDB);

	sqlite3_finalize(dbr);
	method_return_if_fault(env);

	if (vx_bcaps_set(xid, &bcaps) == -1)
		method_return_faultf(env, MESYS, "vx_set_bcaps: %s", strerror(errno));

	/* 1.2) setup context capabilities */
	rc = vxdb_prepare(&dbr, "SELECT ccap FROM vx_ccaps WHERE xid = %d", xid);

	if (rc)
		method_set_fault(env, MEVXDB);

	else
		vxdb_foreach_step(rc, dbr)
			ccaps.flags |= flist64_getval(ccaps_list, sqlite3_column_text(dbr, 0));

	ccaps.mask = ccaps.flags;

	if (rc == -1)
		method_set_fault(env, MEVXDB);

	sqlite3_finalize(dbr);
	method_return_if_fault(env);

	if (vx_ccaps_set(xid, &ccaps) == -1)
		method_return_faultf(env, MESYS, "vx_set_ccaps: %s", strerror(errno));

	/* 1.3) setup context flags */
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

	if (vx_flags_set(xid, &cflags) == -1)
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
	vx_limit_t limit, limit_mask;

	if (vx_limit_mask_get(&limit_mask) == -1)
		method_return_faultf(env, MESYS, "vx_get_limit_mask: %s", strerror(errno));

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

			if (!(limit_mask.softlimit & buf32) &&
					!(limit_mask.maximum   & buf32))
				continue;

			limit.id        = v2i32(buf32);
			limit.softlimit = sqlite3_column_int64(dbr, 1);
			limit.maximum   = sqlite3_column_int64(dbr, 2);

			/* TODO: 0 != infinity */
			if (limit.softlimit == 0)
				limit.softlimit = CRLIM_INFINITY;

			if (limit.maximum == 0)
				limit.maximum = CRLIM_INFINITY;

			if (limit.maximum < limit.softlimit)
				limit.maximum = limit.softlimit;

			if (vx_limit_set(xid, &limit) == -1) {
				method_set_faultf(env, MESYS, "vx_set_limit: %s", strerror(errno));
				break;
			}
		}

		if (rc == -1)
			method_set_fault(env, MEVXDB);
	}

	sqlite3_finalize(dbr);

	return NULL;
}

/* TODO: force VXF_SCHED_HARD/VXF_SCHED_PRIO? */
static
xmlrpc_value *context_scheduler(xmlrpc_env *env)
{
	int rc, cpuid, numcpus;
	vxdb_result *dbr;
	vx_sched_t sched;

	numcpus = sysconf(_SC_NPROCESSORS_ONLN);

	rc = vxdb_prepare(&dbr,
		"SELECT cpuid,fillrate,fillrate2,interval,interval2,"
		"tokensmin,tokensmax "
		"FROM vx_sched WHERE xid = %d ORDER BY cpuid ASC",
		xid);

	if (rc)
		method_set_fault(env, MEVXDB);

	else {
		vxdb_foreach_step(rc, dbr) {
			sched.mask |= VXSM_FILL_RATE|VXSM_INTERVAL;
			sched.mask |= VXSM_TOKENS|VXSM_TOKENS_MIN|VXSM_TOKENS_MAX;

			cpuid = sqlite3_column_int(dbr, 0);

			if (cpuid > numcpus)
				continue;

			if (cpuid >= 0) {
				sched.cpu_id = cpuid;
				sched.mask  |= VXSM_CPU_ID;
			}

			sched.fill_rate[0] = sqlite3_column_int(dbr, 1);
			sched.interval[0]  = sqlite3_column_int(dbr, 2);
			sched.fill_rate[1] = sqlite3_column_int(dbr, 3);
			sched.interval[1]  = sqlite3_column_int(dbr, 4);
			sched.tokens_min   = sqlite3_column_int(dbr, 5);
			sched.tokens_max   = sqlite3_column_int(dbr, 6);

			if (sched.fill_rate[1] > 0 && sched.interval[1] > 0)
				sched.mask |= VXSM_IDLE_TIME|VXSM_FILL_RATE2|VXSM_INTERVAL2;

			if (vx_sched_set(xid, &sched) == -1) {
				method_set_faultf(env, MESYS,
						"vx_set_sched: %s", strerror(errno));
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
xmlrpc_value *context_uname(xmlrpc_env *env, const char *vserverdir)
{
	int rc;
	vxdb_result *dbr;
	uint32_t id;
	vx_uname_t uname;

	rc = vxdb_prepare(&dbr,
		"SELECT uname,value FROM vx_uname WHERE xid = %d",
		xid);

	if (rc)
		method_set_fault(env, MEVXDB);

	else {
		vxdb_foreach_step(rc, dbr) {
			if (!(id = flist32_getval(uname_list, sqlite3_column_text(dbr, 0))))
				continue;

			uname.id = v2i32(id);

			bzero(uname.value, 65);
			memcpy(uname.value, sqlite3_column_text(dbr, 1), 64);

			if (vx_uname_set(xid, &uname) == -1) {
				method_set_faultf(env, MESYS, "vx_set_uname: %s", strerror(errno));
				break;
			}
		}

		if (rc == -1)
			method_set_fault(env, MEVXDB);
	}

	sqlite3_finalize(dbr);
	method_return_if_fault(env);

	if (!str_isempty(name)) {
		uname.id = VHIN_CONTEXT;

		bzero(uname.value, 65);
		snprintf(uname.value, 64, "%s:%s", name, vserverdir);

		if (vx_uname_set(xid, &uname) == -1)
			method_set_faultf(env, MESYS, "vx_set_uname: %s", strerror(errno));
	}

	return NULL;
}

static
xmlrpc_value *namespace_setup(xmlrpc_env *env, const char *vdir)
{
	pid_t pid;
	int status;

	switch ((pid = ns_clone(SIGCHLD, NULL))) {
	case -1:
		method_return_faultf(env, MESYS, "ns_clone: %s", strerror(errno));

	case 0:
		if (chroot_secure_chdir(vdir, "/") == -1)
			log_perror_and_die("chroot_secure_chdir(%s)", vdir);

		/* TODO: do we need RBIND? latest investigation seems to make it
		 * unecessary... if yes, combine vx_clone_namespace and
		 * vx_set_namespace? */
		if (mount(".", "/", NULL, MS_BIND|MS_REC, NULL) == -1)
			log_perror_and_die("mount");

		if (ns_set(xid, 0) == -1)
			log_perror_and_die("ns_set");

		exit(EXIT_SUCCESS);

	default:
		if (waitpid(pid, &status, 0) == -1)
			method_return_faultf(env, MESYS,
					"%s: waitpid: %s", __FUNCTION__, strerror(errno));

		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			method_return_faultf(env, MESYS, "%s", "namespace_setup failed");
	}

	return NULL;
}

static
xmlrpc_value *namespace_mount(xmlrpc_env *env, const char *vdir)
{
	vxdb_result *dbr;
	int rc, mtabfd, status;
	const char *src, *dst, *type, *opts;

	if (ns_enter(xid, 0) == -1)
		method_return_faultf(env, MESYS,
				"vx_enter_namespace: %s", strerror(errno));

	if (chroot_secure_chdir(vdir, "/etc") == -1)
		method_return_faultf(env, MESYS,
				"chroot_secure_chdir: %s", strerror(errno));

	/* TODO: mtab may be a symlink that points outside */
	if ((mtabfd = open_trunc("mtab")) == -1)
		method_return_faultf(env, MESYS, "open_trunc: %s", strerror(errno));

	/* TODO: we only need a root entry if no root is mounted below */
	if (write(mtabfd, "/dev/hdv1 / ufs rw 0 0\n", 23) == -1)
		method_return_faultf(env, MESYS, "write: %s", strerror(errno));

	/* TODO: extra logic for root mount */
	rc = vxdb_prepare(&dbr,
		"SELECT src,dst,type,opts FROM mount WHERE xid = %d",
		xid);

	if (rc)
		method_return_fault(env, MEVXDB);

	else {
		vxdb_foreach_step(rc, dbr) {
			src  = sqlite3_column_text(dbr, 0);
			dst  = sqlite3_column_text(dbr, 1);
			type = sqlite3_column_text(dbr, 2);
			opts = sqlite3_column_text(dbr, 3);

			if (str_isempty(type))
				type = "auto";

			if (str_isempty(opts))
				opts = "defaults";

			if (chroot_secure_chdir(vdir, dst) == -1)
				method_return_faultf(env, MESYS,
						"chroot_secure_chdir: %s", strerror(errno));

			status = exec_fork("/bin/mount -n -t %s -o %s %s .",
					type, opts, src);

			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				method_return_faultf(env, MESYS,
						"mount failed (%d)", WEXITSTATUS(status));

			if (WIFSIGNALED(status))
				method_return_faultf(env, MESYS,
						"mount caught signal (%d)", WTERMSIG(status));

			dprintf(mtabfd, "%s %s %s %s 0 0\n", src, dst, type, opts);
		}

		if (rc == -1)
			method_set_fault(env, MEVXDB);
	}

	sqlite3_finalize(dbr);
	close(mtabfd);

	return NULL;
}

/* helper.startup(int xid) */
xmlrpc_value *m_helper_startup(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

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

	if (vx_info(xid, NULL) == -1)
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

	context_uname(env, vserverdir);
	method_return_if_fault(env);

	namespace_setup(env, vdir);
	method_return_if_fault(env);

	namespace_mount(env, vdir);
	method_return_if_fault(env);

	return xmlrpc_build_value(env, "{s:s,s:s}", "vdir", vdir, "init", init);
}
