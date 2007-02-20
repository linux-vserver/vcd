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
#include <stdlib.h>
#include <inttypes.h>
#include <ftw.h>
#include <limits.h>
#include <search.h>
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
   2) setup disk limits
   3) setup resource limits
   4) setup scheduler
   5) setup unames
   6) setup filesystem namespace
*/

static const char *name = NULL;
static xid_t xid = 0;

/* disk limits variables */
static void *inotable = NULL;
static uint32_t used_blocks = 0;
static uint32_t used_inodes = 0;
static xmlrpc_env *global_env = NULL;

static
xmlrpc_value *context_caps_and_flags(xmlrpc_env *env)
{
	LOG_TRACEME

	int rc;
	vx_flags_t bcaps, ccaps, cflags;

	bcaps.flags = ccaps.flags = cflags.flags = 0;
	bcaps.mask  = ccaps.mask  = ~(0ULL);

	/* 1.1) setup system capabilities */
	rc = vxdb_prepare(&dbr, "SELECT bcap FROM vx_bcaps WHERE xid = %d", xid);

	if (rc == VXDB_OK)
		vxdb_foreach_step(rc, dbr)
			bcaps.flags |= flist64_getval(bcaps_list,
					vxdb_column_text(dbr, 0));

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);
	method_return_if_fault(env);

	log_debug("bcaps(%d): %#.16llx, %#.16llx", xid, bcaps.flags, bcaps.mask);

	if (vx_bcaps_set(xid, &bcaps) == -1)
		method_return_sys_fault(env, "vx_bcaps_set");

	/* 1.2) setup context capabilities */
	rc = vxdb_prepare(&dbr, "SELECT ccap FROM vx_ccaps WHERE xid = %d", xid);

	if (rc == VXDB_OK)
		vxdb_foreach_step(rc, dbr)
			ccaps.flags |= flist64_getval(ccaps_list,
					vxdb_column_text(dbr, 0));

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);
	method_return_if_fault(env);

	log_debug("ccaps(%d): %#.16llx, %#.16llx", xid, ccaps.flags, ccaps.mask);

	if (vx_ccaps_set(xid, &ccaps) == -1)
		method_return_sys_fault(env, "vx_ccaps_set");

	/* 1.3) setup context flags */
	rc = vxdb_prepare(&dbr, "SELECT flag FROM vx_flags WHERE xid = %d", xid);

	if (rc == VXDB_OK)
		vxdb_foreach_step(rc, dbr)
			cflags.flags |= flist64_getval(cflags_list,
					vxdb_column_text(dbr, 0));

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);
	method_return_if_fault(env);

	cflags.mask = cflags.flags;

	log_debug("cflags(%d): %#.16" PRIu64 ", %#.16" PRIu64,
			xid, cflags.flags, cflags.mask);

	if (vx_flags_set(xid, &cflags) == -1)
		method_return_sys_fault(env, "vx_flags_set");

	return NULL;
}

static
int inocmp(const void *a, const void *b)
{
	if (*(const ino_t *)a < *(const ino_t *)b) return -1;
	if (*(const ino_t *)a > *(const ino_t *)b) return  1;
	return 0;
}

static
void inofree(void *nodep)
{
	return;
}

static
int handle_file(const char *fpath, const struct stat *sb,
		int typeflag, struct FTW *ftwbuf)
{
	ix_attr_t attr;

	attr.filename = fpath + ftwbuf->base;

	if (ix_attr_get(&attr) == -1) {
		method_set_sys_faultf(global_env, "ix_attr_get(%s)", fpath);
		return FTW_STOP;
	}

	if (!(attr.flags & IATTR_TAG))
		return FTW_CONTINUE;

	if (sb->st_nlink == 1 || tfind(&sb->st_ino, &inotable, inocmp) == NULL) {
		if (attr.xid == xid) {
			used_blocks += sb->st_blocks;
			used_inodes += 1;
		}

		if (tsearch(&sb->st_ino, &inotable, inocmp) == NULL) {
			method_set_sys_faultf(global_env, "tsearch(%lu)", sb->st_ino);
			return FTW_STOP;
		}
	}

	return FTW_CONTINUE;
}

static
xmlrpc_value *context_disk_limits(xmlrpc_env *env)
{
	LOG_TRACEME

	int rc;
	dx_limit_t dlim;
	const char *vdir = vxdb_getvdir(name);

	dlim.filename = vdir;

	/* always remove the disk limit first */
	log_debug("dx_limit_remove(%d): %s", xid, dlim.filename);

	dx_limit_remove(xid, &dlim);

	rc = vxdb_prepare(&dbr,
			"SELECT space,inodes,reserved FROM dx_limit WHERE xid = %d", xid);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	rc = vxdb_step(dbr);

	if (rc == VXDB_ROW) {
		/* add the disk limit */
		log_debug("dx_limit_add(%d): %s", xid, dlim.filename);

		if (dx_limit_add(xid, &dlim) == -1)
			method_return_sys_fault(env, "dx_limit_add");

		/* get all the needed data for the disk limit */
		uint32_t total_space  = vxdb_column_uint32(dbr, 0);
		uint32_t total_inodes = vxdb_column_uint32(dbr, 1);
		int reserved          = vxdb_column_int(dbr, 2);

		used_blocks = used_inodes = 0;

		if (inotable)
			tdestroy(inotable, inofree);

		inotable = NULL;

		global_env = env;
		nftw(vdir, handle_file, 50, FTW_MOUNT|FTW_PHYS|FTW_CHDIR|FTW_ACTIONRETVAL);
		global_env = NULL;
		method_return_if_fault(env);

		uint32_t used_space = (used_blocks / 2);

		if (total_space < used_space)
			total_space = used_space;

		if (total_inodes < used_inodes)
			total_inodes = used_inodes;

		dlim.space_used   = used_space;
		dlim.space_total  = total_space;
		dlim.inodes_used  = used_inodes;
		dlim.inodes_total = total_inodes;
		dlim.reserved     = reserved;

		/* finally set the disk limit values */
		log_debug("dx_limit_set(%d): %s,%" PRIu32 ",%" PRIu32
				",%" PRIu32 ",%" PRIu32 ",%d",
				xid, dlim.filename,
				dlim.space_used, dlim.space_total,
				dlim.inodes_used, dlim.inodes_total,
				dlim.reserved);

		if (dx_limit_set(xid, &dlim) == -1)
			method_set_sys_fault(env, "dx_limit_set");
	}

	else if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);

	return NULL;
}

static
xmlrpc_value *context_resource_limits(xmlrpc_env *env)
{
	LOG_TRACEME

	int rc;
	const char *type;
	uint32_t buf32;
	vx_limit_t limit, limit_mask;

	if (vx_limit_mask_get(&limit_mask) == -1)
		method_return_sys_fault(env, "vx_limit_mask_get");

	rc = vxdb_prepare(&dbr,
			"SELECT type,soft,max FROM vx_limit WHERE xid = %d",
			xid);

	if (rc == VXDB_OK) {
		vxdb_foreach_step(rc, dbr) {
			type = vxdb_column_text(dbr, 0);

			if (!(buf32 = flist32_getval(rlimit_list, type)))
				continue;

			if (!(limit_mask.softlimit & buf32) &&
					!(limit_mask.maximum & buf32))
				continue;

			limit.id        = v2i32(buf32);
			limit.softlimit = vxdb_column_uint64(dbr, 1);
			limit.maximum   = vxdb_column_uint64(dbr, 2);

			if (limit.maximum < limit.softlimit)
				limit.maximum = limit.softlimit;

			log_debug("rlimit(%d): %" PRIu64 ",%" PRIu64,
					xid, limit.softlimit, limit.maximum);

			if (vx_limit_set(xid, &limit) == -1) {
				method_set_sys_fault(env, "vx_limit_set");
				break;
			}
		}
	}

	if (!env->fault_occurred && rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);

	return NULL;
}

static
xmlrpc_value *context_scheduler(xmlrpc_env *env)
{
	LOG_TRACEME

	int rc, cpuid, numcpus;
	vx_sched_t sched;

	numcpus = sysconf(_SC_NPROCESSORS_ONLN);

	rc = vxdb_prepare(&dbr,
			"SELECT cpuid,fillrate,fillrate2,interval,interval2,"
			"tokensmin,tokensmax "
			"FROM vx_sched WHERE xid = %d ORDER BY cpuid ASC",
			xid);

	if (rc == VXDB_OK) {
		vxdb_foreach_step(rc, dbr) {
			sched.mask |= VXSM_FORCE;
			sched.mask |= VXSM_FILL_RATE|VXSM_INTERVAL;
			sched.mask |= VXSM_TOKENS_MIN|VXSM_TOKENS_MAX;

			cpuid = vxdb_column_int(dbr, 0);

			if (cpuid >= numcpus)
				continue;

			if (cpuid >= 0) {
				sched.cpu_id = cpuid;
				sched.mask  |= VXSM_CPU_ID;
			}

			sched.fill_rate[0] = vxdb_column_int32(dbr, 1);
			sched.fill_rate[1] = vxdb_column_int32(dbr, 2);
			sched.interval[0]  = vxdb_column_int32(dbr, 3);
			sched.interval[1]  = vxdb_column_int32(dbr, 4);
			sched.tokens_min   = vxdb_column_int32(dbr, 5);
			sched.tokens_max   = vxdb_column_int32(dbr, 6);
			sched.prio_bias    = 0;

			if (sched.fill_rate[1] > 0 && sched.interval[1] > 0)
				sched.mask |= VXSM_IDLE_TIME|VXSM_FILL_RATE2|VXSM_INTERVAL2;

			log_debug("sched(%d, %d, %#.16llx): "
					"%" PRIu32 ",%" PRIu32 ",%" PRIu32
					",%" PRIu32 ",%" PRIu32 ",%" PRIu32,
					xid, sched.cpu_id, sched.mask,
					sched.fill_rate[0], sched.interval[0],
					sched.fill_rate[1], sched.interval[1],
					sched.tokens_min, sched.tokens_max);

			if (vx_sched_set(xid, &sched) == -1) {
				method_set_sys_fault(env, "vx_sched_set");
				break;
			}
		}
	}

	if (!env->fault_occurred && rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);
	return NULL;
}

static
xmlrpc_value *context_uname(xmlrpc_env *env, const char *vdir)
{
	LOG_TRACEME

	int rc;
	uint32_t id;
	vx_uname_t uname;

	rc = vxdb_prepare(&dbr,
			"SELECT type,value FROM vx_uname WHERE xid = %d",
			xid);

	if (rc != VXDB_OK)
		method_return_vxdb_fault(env);

	vxdb_foreach_step(rc, dbr) {
		if (!(id = flist32_getval(uname_list, vxdb_column_text(dbr, 0))))
			continue;

		uname.id = v2i32(id);

		mem_set(uname.value, 0, 65);
		mem_cpy(uname.value, vxdb_column_text(dbr, 1), 64);

		log_debug("uname(%d, %d): %s", xid, uname.id, uname.value);

		if (vx_uname_set(xid, &uname) == -1) {
			method_set_sys_fault(env, "vx_uname_set");
			break;
		}
	}

	if (!env->fault_occurred && rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);
	method_return_if_fault(env);

	if (!str_isempty(name)) {
		uname.id = VHIN_CONTEXT;
		snprintf(uname.value, 65, "%s:%s", name, vdir);

		log_debug("uname(%d, %d): %s", xid, uname.id, uname.value);

		if (vx_uname_set(xid, &uname) == -1)
			method_return_sys_fault(env, "vx_uname_set");
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
xmlrpc_value *do_mount(xmlrpc_env *env, const char *vdir,
		vxdb_result *dbr, int mtabfd)
{
	const char *src  = vxdb_column_text(dbr, 0);
	const char *dst  = vxdb_column_text(dbr, 1);
	const char *type = vxdb_column_text(dbr, 2);
	const char *opts = vxdb_column_text(dbr, 3);

	if (str_isempty(type))
		type = "auto";

	if (str_isempty(opts))
		opts = "defaults";

	log_debug("mount(%d): %s %s %s %s", xid, src, dst, type, opts);

	pid_t pid;
	int status;

	switch ((pid = fork())) {
	case -1:
		method_return_sys_fault(env, "fork");

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
					"command '/bin/mount -n -t %s -o %s %s %s' "
					"caught signal: %s",
					type, opts, src, dst, strsignal(WTERMSIG(status)));

		if (mtabfd != -1)
			dprintf(mtabfd, "%s %s %s %s 0 0\n", src, dst, type, opts);
	}

	return NULL;
}

static
xmlrpc_value *namespace_mount(xmlrpc_env *env, const char *vdir)
{
	LOG_TRACEME

	int rc, mtabfd;

	log_debug("vdir(%d): %s", xid, vdir);

	/* mount root entry */
	rc = vxdb_prepare(&dbr,
			"SELECT src,dst,type,opts FROM mount WHERE xid = %d AND dst = '/'",
			xid);

	if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
		do_mount(env, vdir, dbr, -1);

	vxdb_finalize(dbr);
	method_return_if_fault(env);

	if (chroot_secure_chdir(vdir, "/etc") == -1)
		method_return_sys_fault(env, "chroot_secure_chdir(/etc)");

	/* make sure mtab does not exist to prevent symlink attacks
	 * FIXME: what if another process created a new one meanwhile? */
	unlink("mtab");

	if ((mtabfd = open_trunc("mtab")) == -1)
		method_return_sys_fault(env, "open_trunc(/etc/mtab)");

	/* write root entry to mtab */
	dprintf(mtabfd, "/dev/root / ufs defaults 0 0\n");

	/* mount non-root entries */
	rc = vxdb_prepare(&dbr,
			"SELECT src,dst,type,opts FROM mount WHERE xid = %d",
			xid);

	if (rc == VXDB_OK) {
		vxdb_foreach_step(rc, dbr) {
			do_mount(env, vdir, dbr, mtabfd);

			if (env->fault_occurred)
				break;
		}
	}

	if (!env->fault_occurred && rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);
	close(mtabfd);

	return NULL;
}

/* helper.startup(int xid) */
xmlrpc_value *m_helper_startup(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	const char *vdir, *init = "/sbin/init";
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

	if (rc == VXDB_OK)
		vxdb_foreach_step(rc, dbr)
			init = str_dup(vxdb_column_text(dbr, 0));

	if (rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	vxdb_finalize(dbr);
	method_return_if_fault(env);

	context_caps_and_flags(env);
	method_return_if_fault(env);

	context_disk_limits(env);
	method_return_if_fault(env);

	context_resource_limits(env);
	method_return_if_fault(env);

	context_scheduler(env);
	method_return_if_fault(env);

	namespace_setup(env);
	method_return_if_fault(env);

	context_uname(env, vdir);
	method_return_if_fault(env);

	namespace_mount(env, vdir);
	method_return_if_fault(env);

	return xmlrpc_build_value(env, "{s:s,s:s}", "vdir", vdir, "init", init);
}
