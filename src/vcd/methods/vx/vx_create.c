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
#include <ftw.h>
#include <confuse.h>

#include "auth.h"
#include "cfg.h"
#include "lists.h"
#include "methods.h"
#include "vxdb.h"

#include <lucid/chroot.h>
#include <lucid/flist.h>
#include <lucid/mem.h>
#include <lucid/misc.h>
#include <lucid/log.h>
#include <lucid/open.h>
#include <lucid/str.h>
#include <lucid/stralloc.h>

#define DEFAULT_BCAPS "{ LINUX_IMMUTABLE, NET_BROADCAST, NET_ADMIN, NET_RAW, " \
                      "IPC_LOCK, IPC_OWNER, SYS_MODULE, SYS_RAWIO, " \
                      "SYS_PACCT, SYS_ADMIN, SYS_NICE, SYS_RESOURCE, " \
                      "SYS_TIME, MKNOD, AUDIT_CONTROL }"

#define DEFAULT_CCAPS "{ SET_UTSNAME, RAW_ICMP }"

#define DEFAULT_FLAGS "{ VIRT_MEM, VIRT_UPTIME, VIRT_CPU, " \
                      "VIRT_LOAD, HIDE_NETIF }"

static cfg_opt_t BUILD_OPTS[] = {
	CFG_STR("description", NULL, CFGF_NONE),

	CFG_STR("init",    "/sbin/init",   CFGF_NONE),
	CFG_STR("halt",    "/sbin/halt",   CFGF_NONE),
	CFG_STR("reboot",  "/sbin/reboot", CFGF_NONE),
	CFG_INT("timeout", 15,             CFGF_NONE),

	CFG_STR_LIST("vx_bcaps", DEFAULT_BCAPS, CFGF_NONE),
	CFG_STR_LIST("vx_ccaps", DEFAULT_CCAPS, CFGF_NONE),
	CFG_STR_LIST("vx_flags", DEFAULT_FLAGS, CFGF_NONE),
	CFG_END()
};

static xid_t xid = 0;
static char *name = NULL;

static char *vdir = NULL;
static int vdirfd = -1;

static char *template = NULL;
static char *tdir = NULL;

static int force = 0;

static xmlrpc_env *global_env = NULL;

static
int handle_file(const char *fpath, const struct stat *sb,
		int typeflag, struct FTW *ftwbuf)
{
	char *src;

	ix_attr_t attr = {
		.filename = NULL,
		.xid   = xid,
		.flags = IATTR_IUNLINK|IATTR_IMMUTABLE,
		.mask  = IATTR_IUNLINK|IATTR_IMMUTABLE,
	};

	/* skip vdir */
	if (ftwbuf->level < 1)
		return FTW_CONTINUE;

	/* skip character/block devices, fifos and sockets */
	if (S_ISCHR(sb->st_mode) ||
			S_ISBLK(sb->st_mode) ||
			S_ISFIFO(sb->st_mode) ||
			S_ISSOCK(sb->st_mode))
		return FTW_CONTINUE;

	/* remember */
	int curfd = open_read(".");

	if (fchdir(vdirfd) == -1) {
		method_set_sys_fault(global_env, "fchdir");
		return FTW_STOP;
	}

	switch (typeflag) {
	case FTW_D:
		/* create new directory */
		if (mkdir(fpath, sb->st_mode) == -1) {
			method_set_sys_faultf(global_env, "mkdir(%s)", fpath);
			return FTW_STOP;
		}

		if (lchown(fpath, sb->st_uid, sb->st_gid) == -1) {
			method_set_sys_faultf(global_env, "lchown(%s)", fpath);
			return FTW_STOP;
		}

		fchdir(curfd);
		close(curfd);
		return FTW_CONTINUE;

	case FTW_F:
	case FTW_SL:
		src = str_path_concat(tdir, fpath);

		if (str_isempty(src)) {
			method_set_faultf(global_env, MEINVAL,
					"%s: invalid src path: %s/%s", __FUNCTION__, tdir, fpath);
			return FTW_STOP;
		}

		attr.filename = src;

		/* link file */
		if (ix_attr_set(&attr) == -1) {
			method_set_sys_faultf(global_env, "ix_attr_set(%s)", fpath);
			return FTW_STOP;
		}

		if (link(src, fpath) == -1) {
			method_set_sys_faultf(global_env, "link(%s, %s)", src, fpath);
			return FTW_STOP;
		}

		mem_free(src);

		fchdir(curfd);
		close(curfd);
		return FTW_CONTINUE;

	default:
		method_set_faultf(global_env, MESYS,
				"nftw returned %d on %s", typeflag, fpath);
	}

	return FTW_STOP;
}

static
xmlrpc_value *link_unified_root(xmlrpc_env *env)
{
	if (chroot_secure_chdir(vdir, "/") == -1)
		method_return_sys_fault(env, "chroot_secure_chdir");

	/* handle_file uses this to make sure we are in vdir */
	vdirfd = open_read(".");

	global_env = env;
	chdir(tdir);
	nftw(".", handle_file, 50, FTW_ACTIONRETVAL|FTW_PHYS);
	global_env = NULL;

	close(vdirfd);
	return NULL;
}

static
xmlrpc_value *build_root_filesystem(xmlrpc_env *env)
{
	LOG_TRACEME

	/* try to get rid of old vdir if this is a rebuild */
	if (xid > 0) {
		const char *oldvdir = vxdb_getvdir(name);

		if (str_isempty(oldvdir) || !str_path_isabs(oldvdir))
			method_return_faultf(env, MECONF,
					"invalid old vdir: %s", oldvdir);

		runlink(oldvdir);
	}

	/* try to runlink vdir first */
	runlink(vdir);

	/* create new vdir */
	if (mkdirp(vdir, 0755) == -1)
		method_return_sys_fault(env, "mkdirp");

	/* now link the template to new vdir */
	link_unified_root(env);

	/* runlink on failure */
	if (env->fault_occurred) {
		runlink(vdir);
		return NULL;
	}

	/* sanitize device nodes */
	if (chroot_secure_chdir(vdir, "/") == -1)
		method_return_sys_fault(env, "chroot_secure_chdir");

	if (runlink("dev") == -1 ||
			mkdir("dev", 0755) == -1 ||
			mkdir("dev/pts", 0755) == -1 ||
			mknod("dev/null",    0666 | S_IFCHR, makedev(1,3)) == -1 ||
			mknod("dev/zero",    0666 | S_IFCHR, makedev(1,5)) == -1 ||
			mknod("dev/full",    0666 | S_IFCHR, makedev(1,7)) == -1 ||
			mknod("dev/random",  0644 | S_IFCHR, makedev(1,8)) == -1 ||
			mknod("dev/urandom", 0644 | S_IFCHR, makedev(1,9)) == -1 ||
			mknod("dev/tty",     0666 | S_IFCHR, makedev(5,0)) == -1 ||
			mknod("dev/ptmx",    0666 | S_IFCHR, makedev(5,2)) == -1)
		method_return_faultf(env, MESYS,
				"could not sanitize /dev: %s", strerror(errno));

	return NULL;
}

static
xmlrpc_value *find_free_xid(xmlrpc_env *env)
{
	LOG_TRACEME

	vxdb_result *dbr;

	if (xid != 0)
		return NULL;

	int rc = vxdb_prepare(&dbr,
			"SELECT COUNT(xid),MAX(xid) FROM xid_name_map");

	if (rc == SQLITE_OK) {
		vxdb_foreach_step(rc, dbr) {
			int cnt = sqlite3_column_int(dbr, 0);
			int max = sqlite3_column_int(dbr, 1);

			if (max < 2)
				xid = 2;

			else if (max < 65535)
				xid = max + 1;
	
			else if (cnt < 65535) {
				int i;
	
				for (i = 2; i < 65535; i++) {
					if (!vxdb_getname(i)) {
						xid = i;
						break;
					}
				}
			}
		}
	}

	if (!env->fault_occurred && rc != SQLITE_DONE)
		method_set_vxdb_fault(env);

	if (xid == 0)
			method_set_faultf(env, MEVXDB,
					"no free context id available: %s", name);

	sqlite3_finalize(dbr);
	return NULL;
}

static
xmlrpc_value *create_vxdb_entries(xmlrpc_env *env)
{
	LOG_TRACEME

	char *tconf;
	if (vasprintf(&tconf, "%s.conf", tdir) < 1)
		method_return_sys_fault(env, "vasprintf");

	if (!isfile(tconf))
		tconf = "/dev/null";

	cfg_t *tcfg = cfg_init(BUILD_OPTS, CFGF_NOCASE);

	switch (cfg_parse(tcfg, tconf)) {
	case CFG_FILE_ERROR:
		method_return_faultf(env, MECONF,
				"could not read template configuration: %s", template);

	case CFG_PARSE_ERROR:
		method_return_faultf(env, MECONF,
				"syntax error in template configuration: %s", template);

	default:
		break;
	}

	/* find a free xid */
	find_free_xid(env);
	method_return_if_fault(env);

	/* assemble SQL query */
	stralloc_t _sa, *sa = &_sa;
	stralloc_init(sa);
	stralloc_cats(sa, "BEGIN EXCLUSIVE TRANSACTION;");

	/* remove any existing stuff */
	stralloc_catf(sa,
			"DELETE FROM dx_limit     WHERE xid = %d;"
			"DELETE FROM init         WHERE xid = %d;"
			"DELETE FROM mount        WHERE xid = %d;"
			"DELETE FROM nx_addr      WHERE xid = %d;"
			"DELETE FROM nx_broadcast WHERE xid = %d;"
			"DELETE FROM reboot       WHERE xid = %d;"
			"DELETE FROM vdir         WHERE xid = %d;"
			"DELETE FROM vx_bcaps     WHERE xid = %d;"
			"DELETE FROM vx_ccaps     WHERE xid = %d;"
			"DELETE FROM vx_flags     WHERE xid = %d;"
			"DELETE FROM vx_limit     WHERE xid = %d;"
			"DELETE FROM vx_sched     WHERE xid = %d;"
			"DELETE FROM vx_uname     WHERE xid = %d;"
			"DELETE FROM xid_name_map WHERE xid = %d;", /* 14 */
			xid, xid, xid, xid, xid, xid, xid,
			xid, xid, xid, xid, xid, xid, xid);

	/* get init configuration */
	const char *init   = cfg_getstr(tcfg, "init");
	const char *halt   = cfg_getstr(tcfg, "halt");
	const char *reboot = cfg_getstr(tcfg, "reboot");
	int timeout        = cfg_getint(tcfg, "timeout");

	if (str_isempty(init) || !str_path_isabs(init))
		method_return_faultf(env, MECONF,
				"invalid template configuration for init: %s", init);

	if (str_isempty(halt) || !str_path_isabs(halt))
		method_return_faultf(env, MECONF,
				"invalid template configuration for halt: %s", halt);

	if (str_isempty(reboot) || !str_path_isabs(reboot))
		method_return_faultf(env, MECONF,
				"invalid template configuration for reboot: %s", reboot);

	timeout = timeout < 0 ? 0 : timeout;

	stralloc_catf(sa,
			"INSERT INTO init (xid, init, halt, reboot, timeout) "
			"VALUES (%d, '%s', '%s', '%s', %d);",
			xid, init, halt, reboot, timeout);

	int i;

	/* get bcaps */
	int vx_bcaps_size = cfg_size(tcfg, "vx_bcaps");

	for (i = 0; i < vx_bcaps_size; i++) {
		const char *bcap = cfg_getnstr(tcfg, "vx_bcaps", i);

		if (!flist64_getval(bcaps_list, bcap))
			method_return_faultf(env, MECONF,
					"invalid template configuration for bcap: %s", bcap);

		stralloc_catf(sa,
				"INSERT INTO vx_bcaps (xid, bcap) "
				"VALUES (%d, '%s');",
				xid, bcap);
	}

	/* get ccaps */
	int vx_ccaps_size = cfg_size(tcfg, "vx_ccaps");

	for (i = 0; i < vx_ccaps_size; i++) {
		const char *ccap = cfg_getnstr(tcfg, "vx_ccaps", i);

		if (!flist64_getval(ccaps_list, ccap))
			method_return_faultf(env, MECONF,
					"invalid template configuration for ccap: %s", ccap);

		stralloc_catf(sa,
				"INSERT INTO vx_ccaps (xid, ccap) "
				"VALUES (%d, '%s');",
				xid, ccap);
	}

	/* get cflags */
	int vx_flags_size = cfg_size(tcfg, "vx_flags");

	for (i = 0; i < vx_flags_size; i++) {
		const char *flag = cfg_getnstr(tcfg, "vx_flags", i);

		if (!flist64_getval(cflags_list, flag))
			method_return_faultf(env, MECONF,
					"invalid template configuration for cflag: %s", flag);

		stralloc_catf(sa,
				"INSERT INTO vx_flags (xid, flag) "
				"VALUES (%d, '%s');",
				xid, flag);
	}

	/* insert name */
	stralloc_catf(sa,
			"INSERT INTO xid_name_map (xid, name) VALUES (%d, '%s');",
			xid, name);

	/* commit transaction */
	stralloc_cats(sa, "COMMIT TRANSACTION;");

	char *sql = stralloc_finalize(sa);

	if (vxdb_exec(sql) != SQLITE_OK) {
		method_set_vxdb_fault(env);
		runlink(vdir);
	}

	return NULL;
}

/* vx.create(string name, string template, bool force[, string vdir]) */
xmlrpc_value *m_vx_create(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;

	params = method_init(env, p, c, VCD_CAP_CREATE, M_LOCK);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,s:b,s:s,*}",
			"name", &name,
			"template", &template,
			"force", &force,
			"vdir", &vdir);
	method_return_if_fault(env);

	/* get template dir */
	if (str_isempty(template) || !str_isgraph(template))
		method_return_faultf(env, MEINVAL,
				"invalid template name: %s", template);

	const char *tbasedir = cfg_getstr(cfg, "templatedir");

	if (!str_path_isabs(tbasedir) || !isdir(tbasedir))
		method_return_faultf(env, MECONF,
				"tbasedir does not exist: %s", tbasedir);

	if (!(tdir = str_path_concat(tbasedir, template)))
		method_return_faultf(env, MEINVAL,
				"invalid template path: %s/%s", tbasedir, template);

	if (!isdir(tdir))
		method_return_faultf(env, MEINVAL,
				"template does not exist: %s", template);

	/* check vdir */
	if (str_isempty(vdir))
		vdir = vxdb_getvdir(name);

	if (!str_path_isabs(vdir))
		method_return_faultf(env, MEINVAL,
				"invalid vdir: %s", vdir);

	if (!force && ispath(vdir))
		method_return_faultf(env, MEEXIST,
				"vdir already exists: %s", vdir);

	/* get old xid if rebuild */
	if ((xid = vxdb_getxid(name))) {
		if (!force)
			method_return_fault(env, MEEXIST);
		else if (vx_info(xid, NULL) == 0)
			method_return_fault(env, MERUNNING);
	}

	build_root_filesystem(env);
	method_return_if_fault(env);

	create_vxdb_entries(env);
	method_return_if_fault(env);

	return xmlrpc_nil_new(env);
}
