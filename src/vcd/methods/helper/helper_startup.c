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
#include <inttypes.h>
#include <limits.h>
#include <sys/wait.h>

#include "auth.h"
#include "cfg.h"
#include "lists.h"
#include "methods.h"
#include "vxdb.h"

#include <lucid/bitmap.h>
#include <lucid/chroot.h>
#include <lucid/exec.h>
#include <lucid/log.h>
#include <lucid/mem.h>
#include <lucid/open.h>
#include <lucid/printf.h>
#include <lucid/scanf.h>
#include <lucid/str.h>

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
	LOG_TRACEME

	int rc;
	vxdb_result *dbr;
	vx_flags_t bcaps, ccaps, cflags;

	bcaps.flags = ccaps.flags = cflags.flags = 0;
	bcaps.mask  = ccaps.mask  = ~(0ULL);

	/* 1.1) setup system capabilities */
	rc = vxdb_prepare(&dbr, "SELECT bcap FROM vx_bcaps WHERE xid = %d", xid);

	if (rc == SQLITE_OK)
		vxdb_foreach_step(rc, dbr)
			bcaps.flags |= flist64_getval(bcaps_list,
					sqlite3_column_text(dbr, 0));

	if (rc != SQLITE_DONE)
		method_set_vxdb_fault(env);

	sqlite3_finalize(dbr);
	method_return_if_fault(env);

	bcaps.flags = ~(bcaps.flags);

	log_debug("bcaps(%d): %#.16llx, %#.16llx", xid, bcaps.flags, bcaps.mask);

	if (vx_bcaps_set(xid, &bcaps) == -1)
		method_return_sys_fault(env, "vx_set_bcaps");

	/* 1.2) setup context capabilities */
	rc = vxdb_prepare(&dbr, "SELECT ccap FROM vx_ccaps WHERE xid = %d", xid);

	if (rc == SQLITE_OK)
		vxdb_foreach_step(rc, dbr)
			ccaps.flags |= flist64_getval(ccaps_list,
					sqlite3_column_text(dbr, 0));

	if (rc != SQLITE_DONE)
		method_set_vxdb_fault(env);

	sqlite3_finalize(dbr);
	method_return_if_fault(env);

	log_debug("ccaps(%d): %#.16llx, %#.16llx", xid, ccaps.flags, ccaps.mask);

	if (vx_ccaps_set(xid, &ccaps) == -1)
		method_return_sys_fault(env, "vx_set_ccaps");

	/* 1.3) setup context flags */
	rc = vxdb_prepare(&dbr, "SELECT flag FROM vx_flags WHERE xid = %d", xid);

	if (rc == SQLITE_OK)
		vxdb_foreach_step(rc, dbr)
			cflags.flags |= flist64_getval(cflags_list,
					sqlite3_column_text(dbr, 0));

	if (rc != SQLITE_DONE)
		method_set_vxdb_fault(env);

	sqlite3_finalize(dbr);
	method_return_if_fault(env);

	cflags.mask = cflags.flags;

	log_debug("cflags(%d): %#.16" PRIu64 ", %#.16" PRIu64,
			xid, cflags.flags, cflags.mask);

	if (vx_flags_set(xid, &cflags) == -1)
		method_return_sys_fault(env, "vx_set_flags");

	return NULL;
}

static
uint64_t str_to_rlim(const char *str)
{
	if (str_cmp(str, "inf") == 0)
		return CRLIM_INFINITY;

	uint64_t lim;

	/* if the argument is invalid we keep defaults */
	if (sscanf(str, "%" SCNu64, &lim) < 1)
		return CRLIM_KEEP;

	return lim;
}

static
xmlrpc_value *context_resource_limits(xmlrpc_env *env)
{
	LOG_TRACEME

	vxdb_result *dbr;
	int rc;
	const char *type;
	uint32_t buf32;
	vx_limit_t limit, limit_mask;

	if (vx_limit_mask_get(&limit_mask) == -1)
		method_return_sys_fault(env, "vx_get_limit_mask");

	rc = vxdb_prepare(&dbr,
			"SELECT type,soft,max FROM vx_limit WHERE xid = %d",
			xid);

	if (rc == SQLITE_OK) {
		vxdb_foreach_step(rc, dbr) {
			type = sqlite3_column_text(dbr, 0);

			if (!(buf32 = flist32_getval(rlimit_list, type)))
				continue;

			if (!(limit_mask.softlimit & buf32) &&
					!(limit_mask.maximum & buf32))
				continue;

			limit.id        = v2i32(buf32);
			limit.softlimit = str_to_rlim(sqlite3_column_text(dbr, 1));
			limit.maximum   = str_to_rlim(sqlite3_column_text(dbr, 2));

			if (limit.maximum < limit.softlimit)
				limit.maximum = limit.softlimit;

			log_debug("rlimit(%d): %" PRIu64 ",%" PRIu64,
					xid, limit.softlimit, limit.maximum);

			if (vx_limit_set(xid, &limit) == -1) {
				method_set_sys_fault(env, "vx_set_limit");
				break;
			}
		}
	}

	if (!env->fault_occurred && rc != SQLITE_DONE)
		method_set_vxdb_fault(env);

	sqlite3_finalize(dbr);
	return NULL;
}

static
xmlrpc_value *context_scheduler(xmlrpc_env *env)
{
	LOG_TRACEME

	int rc, cpuid, numcpus;
	vxdb_result *dbr;
	vx_sched_t sched;

	numcpus = sysconf(_SC_NPROCESSORS_ONLN);

	rc = vxdb_prepare(&dbr,
			"SELECT cpuid,fillrate,fillrate2,interval,interval2,"
			"tokensmin,tokensmax "
			"FROM vx_sched WHERE xid = %d ORDER BY cpuid ASC",
			xid);

	if (rc == SQLITE_OK) {
		vxdb_foreach_step(rc, dbr) {
			sched.mask |= VXSM_FILL_RATE|VXSM_INTERVAL;
			sched.mask |= VXSM_TOKENS_MIN|VXSM_TOKENS_MAX;

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

			log_debug("sched(%d, %d, %#.16llx): "
					"%llu, %llu, %llu, %llu, %llu, %llu",
					xid, cpuid, sched.mask,
					sched.fill_rate[0], sched.interval[0],
					sched.fill_rate[1], sched.interval[1],
					sched.tokens_min, sched.tokens_max);

			if (vx_sched_set(xid, &sched) == -1) {
				method_set_sys_fault(env, "vx_set_sched");
				break;
			}
		}
	}

	if (!env->fault_occurred && rc != SQLITE_DONE)
		method_set_vxdb_fault(env);

	sqlite3_finalize(dbr);
	return NULL;
}

static
xmlrpc_value *context_uname(xmlrpc_env *env, const char *vdir)
{
	LOG_TRACEME

	int rc;
	vxdb_result *dbr;
	uint32_t id;
	vx_uname_t uname;

	rc = vxdb_prepare(&dbr,
			"SELECT uname,value FROM vx_uname WHERE xid = %d",
			xid);

	if (rc != SQLITE_OK)
		method_return_vxdb_fault(env);

	vxdb_foreach_step(rc, dbr) {
		if (!(id = flist32_getval(uname_list, sqlite3_column_text(dbr, 0))))
			continue;

		uname.id = v2i32(id);

		mem_set(uname.value, 0, 65);
		mem_cpy(uname.value, sqlite3_column_text(dbr, 1), 64);

		log_debug("uname(%d, %d): %s", xid, uname.id, uname.value);

		if (vx_uname_set(xid, &uname) == -1) {
			method_set_sys_fault(env, "vx_set_uname");
			break;
		}
	}

	if (rc != SQLITE_DONE)
		method_set_vxdb_fault(env);

	sqlite3_finalize(dbr);
	method_return_if_fault(env);

	if (!str_isempty(name)) {
		uname.id = VHIN_CONTEXT;
		snprintf(uname.value, 65, "%s:%s", name, vdir);

		log_debug("uname(%d, %d): %s", xid, uname.id, uname.value);

		if (vx_uname_set(xid, &uname) == -1)
			method_return_sys_fault(env, "vx_set_uname");
	}

	return NULL;
}

static
xmlrpc_value *namespace_setup(xmlrpc_env *env)
{
	LOG_TRACEME

	pid_t pid;
	int status;

	switch ((pid = ns_clone(SIGCHLD, NULL))) {
	case -1:
		method_return_sys_fault(env, "ns_clone");

	case 0:
		if (ns_set(xid, 0) == -1) {
			log_perror("ns_set");
			exit(errno);
		}

		exit(EXIT_SUCCESS);

	default:
		if (waitpid(pid, &status, 0) == -1)
			method_return_sys_fault(env, "waitpid");

		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) {
			errno = WEXITSTATUS(status);
			method_return_sys_fault(env, "ns_set");
		}
	}

	return NULL;
}

static
xmlrpc_value *do_mount(xmlrpc_env *env, const char *vdir, vxdb_result *dbr, int mtabfd)
{
	const char *src  = sqlite3_column_text(dbr, 0);
	const char *dst  = sqlite3_column_text(dbr, 1);
	const char *type = sqlite3_column_text(dbr, 2);
	const char *opts = sqlite3_column_text(dbr, 3);

	if (str_isempty(type))
		type = "auto";

	if (str_isempty(opts))
		opts = "defaults";

	log_debug("mount(%d): %s %s %s %s", xid, src, dst, type, opts);

	pid_t pid;
	int status;

	switch ((pid = fork())) {
	case -1:
		method_return_sys_fault(env, "ns_clone");

	case 0:
		if (ns_enter(xid, 0) == -1)
			exit(errno);

		if (chroot_secure_chdir(vdir, dst) == -1)
			exit(errno);

		exec_replace("/bin/mount -n -t %s -o %s %s .",
				type, opts, src);

		exit(errno);

	default:
		if (waitpid(pid, &status, 0) == -1)
			method_return_sys_fault(env, "waitpid");

		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			method_return_faultf(env, MEEXEC + WEXITSTATUS(status),
					"command '/bin/mount -n -t %s -o %s %s %s' failed",
					type, opts, src, dst);

		if (WIFSIGNALED(status))
			method_return_faultf(env, MEEXEC,
					"command '/bin/mount -n -t %s -o %s %s %s' caught signal: %s",
					type, opts, src, dst, strsignal(WTERMSIG(status)));

		dprintf(mtabfd, "%s %s %s %s 0 0\n", src, dst, type, opts);
	}

	return NULL;
}


static
xmlrpc_value *namespace_mount(xmlrpc_env *env, const char *vdir)
{
	LOG_TRACEME

	vxdb_result *dbr;
	int rc, mtabfd;

	log_debug("vdir(%d): %s", xid, vdir);

	if (chroot_secure_chdir(vdir, "/etc") == -1)
		method_return_sys_fault(env, "chroot_secure_chdir");

	/* make sure mtab does not exist to prevent symlink attacks */
	unlink("mtab");

	if ((mtabfd = open_trunc("mtab")) == -1)
		method_return_sys_fault(env, "open_trunc");

	/* mount root entry */
	rc = vxdb_prepare(&dbr,
			"SELECT src,dst,type,opts FROM mount WHERE xid = %d AND dst = '/'",
			xid);

	if (rc == SQLITE_OK) {
		vxdb_foreach_step(rc, dbr) {
			do_mount(env, vdir, dbr, mtabfd);

			if (env->fault_occurred)
				break;
		}
	}

	if (!env->fault_occurred && rc != SQLITE_DONE)
		method_set_vxdb_fault(env);

	sqlite3_finalize(dbr);

	if (env->fault_occurred) {
		close(mtabfd);
		return NULL;
	}

	/* mount non-root entries */
	rc = vxdb_prepare(&dbr,
			"SELECT src,dst,type,opts FROM mount WHERE xid = %d",
			xid);

	if (rc == SQLITE_OK) {
		vxdb_foreach_step(rc, dbr) {
			do_mount(env, vdir, dbr, mtabfd);

			if (env->fault_occurred)
				break;
		}
	}

	if (!env->fault_occurred && rc != SQLITE_DONE)
		method_set_vxdb_fault(env);

	sqlite3_finalize(dbr);
	close(mtabfd);

	return NULL;
}

/* helper.startup(int xid) */
xmlrpc_value *m_helper_startup(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	const char *vdir, *init = "/sbin/init";
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

	if (!(vdir = vxdb_getvdir(name)))
		method_return_faultf(env, MECONF, "invalid vdir: %s", vdir);

	rc = vxdb_prepare(&dbr, "SELECT init FROM init WHERE xid = %d", xid);

	if (rc == SQLITE_OK)
		vxdb_foreach_step(rc, dbr)
			init = str_dup(sqlite3_column_text(dbr, 0));

	if (rc != SQLITE_DONE)
		method_set_vxdb_fault(env);

	sqlite3_finalize(dbr);
	method_return_if_fault(env);

	context_caps_and_flags(env);
	method_return_if_fault(env);

	context_resource_limits(env);
	method_return_if_fault(env);

	context_scheduler(env);
	method_return_if_fault(env);

	context_uname(env, vdir);
	method_return_if_fault(env);

	namespace_setup(env);
	method_return_if_fault(env);

	namespace_mount(env, vdir);
	method_return_if_fault(env);

	return xmlrpc_build_value(env, "{s:s,s:s}", "vdir", vdir, "init", init);
}
