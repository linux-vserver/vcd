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
#include <fcntl.h>
#include <ftw.h>
#include <dirent.h>
#include <confuse.h>

#include "auth.h"
#include "cfg.h"
#include "lists.h"
#include "methods.h"
#include "syscall-compat.h"
#include "validate.h"
#include "vxdb.h"

#include <lucid/chroot.h>
#include <lucid/flist.h>
#include <lucid/mem.h>
#include <lucid/misc.h>
#include <lucid/log.h>
#include <lucid/open.h>
#include <lucid/printf.h>
#include <lucid/str.h>
#include <lucid/stralloc.h>

#define DEFAULT_BCAPS "{ LINUX_IMMUTABLE, NET_BROADCAST, NET_ADMIN, NET_RAW, " \
                      "IPC_LOCK, IPC_OWNER, SYS_MODULE, SYS_RAWIO, " \
                      "SYS_PACCT, SYS_ADMIN, SYS_NICE, SYS_RESOURCE, " \
                      "SYS_TIME, MKNOD, AUDIT_CONTROL }"

#define DEFAULT_CCAPS "{ SET_UTSNAME, RAW_ICMP }"

#define DEFAULT_FLAGS "{ VIRT_MEM, VIRT_UPTIME, VIRT_CPU, " \
                      "VIRT_LOAD, HIDE_NETIF }"

static cfg_opt_t MOUNT_OPTS[] = {
	CFG_STR("src", NULL, CFGF_NONE),
	CFG_STR("type", NULL, CFGF_NONE),
	CFG_STR("opts", NULL, CFGF_NONE),
	CFG_END()
};

static cfg_opt_t BUILD_OPTS[] = {
	CFG_STR("description", NULL, CFGF_NONE),

	CFG_STR("init",    "/sbin/init",   CFGF_NONE),
	CFG_STR("halt",    "/sbin/halt",   CFGF_NONE),
	CFG_STR("reboot",  "/sbin/reboot", CFGF_NONE),
	CFG_INT("timeout", 15,             CFGF_NONE),

	CFG_SEC("mount", MOUNT_OPTS, CFGF_MULTI|CFGF_TITLE|CFGF_NONE),

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
static int copy  = 0;

static xmlrpc_env *global_env = NULL;

static
int handle_file(const char *fpath, const struct stat *sb,
		int typeflag, struct FTW *ftwbuf)
{
	char *ffile, *buf;
	int do_chown = 0, do_chmod = 0, do_chxid = 0;

	ffile = str_path_concat(vdir, fpath);

	ix_attr_t attr = {
		.filename = ffile,
		.xid      = xid,
		.flags    = IATTR_TAG,
		.mask     = IATTR_TAG,
	};

	/* skip vdir */
	if (ftwbuf->level < 1)
		return FTW_CONTINUE;

	/* skip character/block devices, fifos and sockets */
	if (S_ISCHR(sb->st_mode)  ||
		S_ISBLK(sb->st_mode)  ||
		S_ISFIFO(sb->st_mode) ||
		S_ISSOCK(sb->st_mode))
		return FTW_CONTINUE;

	switch (typeflag) {
	case FTW_D:
		/* create new directory */
		if (mkdirat(vdirfd, fpath, sb->st_mode) == -1) {
			method_set_sys_faultf(global_env, "mkdirat(%s)", ffile);
			return FTW_STOP;
		}

		do_chown = do_chxid = 1;
		break;

	case FTW_F:
		/* copy file */
		if (copy) {
			int srcfd = open(fpath, O_RDONLY|O_NONBLOCK|O_NOFOLLOW);

			if (srcfd == -1) {
				method_set_sys_faultf(global_env, "open(%s)", fpath);
				return FTW_STOP;
			}

			int dstfd = openat(vdirfd, fpath, O_RDWR|O_CREAT|O_EXCL, 0200);

			if (dstfd == -1) {
				close(srcfd);
				method_set_sys_faultf(global_env, "openat(%s)", fpath);
				return FTW_STOP;
			}

			if (copy_file(srcfd, dstfd) == -1) {
				method_set_sys_faultf(global_env, "copy_file(%s)", fpath);
				return FTW_STOP;
			}

			close(dstfd);
			close(srcfd);

			do_chown = do_chmod = do_chxid = 1;
		}

		/* link file */
		else {
			/* set needed file attributes for linking */
			attr.filename = fpath;
			attr.xid      = 0;
			attr.flags    = IATTR_IUNLINK|IATTR_IMMUTABLE;
			attr.mask     = IATTR_IUNLINK|IATTR_IMMUTABLE;

			if (ix_attr_set(&attr) == -1) {
				method_set_sys_faultf(global_env, "ix_attr_set(%s)", fpath);
				return FTW_STOP;
			}

			if (linkat(AT_FDCWD, fpath, vdirfd, fpath, 0) == -1) {
				method_set_sys_faultf(global_env, "linkat(%s)", fpath);
				return FTW_STOP;
			}
		}

		break;

	case FTW_SL:
		/* copy symlink */
		if (!(buf = readsymlink(fpath))) {
			method_set_sys_faultf(global_env, "readsymlink(%s)", fpath);
			return FTW_STOP;
		}

		if (symlinkat(buf, vdirfd, fpath) == -1) {
			method_set_sys_faultf(global_env, "symlinkat(%s, %s)", buf, fpath);
			return FTW_STOP;
		}

		mem_free(buf);

		do_chown = do_chxid = 1;
		break;

	default:
		method_set_faultf(global_env, MESYS,
				"nftw returned %d on %s", typeflag, fpath);
	}

	if (do_chmod && fchmodat(vdirfd, fpath, sb->st_mode, 0) == -1) {
		method_set_sys_faultf(global_env, "fchmodat(%s)", ffile);
		return FTW_STOP;
	}

	if (do_chown && fchownat(vdirfd, fpath, sb->st_uid,
			sb->st_gid, AT_SYMLINK_NOFOLLOW) == -1) {
		method_set_sys_faultf(global_env, "fchownat(%s)", ffile);
		return FTW_STOP;
	}

	if (do_chxid && ix_attr_set(&attr) == -1) {
		method_set_sys_faultf(global_env, "ix_attr_set(%s)", ffile);
		return FTW_STOP;
	}

	mem_free(ffile);

	return FTW_CONTINUE;
}

static
xmlrpc_value *make_vserver_filesystem(xmlrpc_env *env)
{
	if (chroot_secure_chdir(vdir, "/") == -1)
		method_return_sys_fault(env, "chroot_secure_chdir");

	/* handle_file uses this to make sure we are in the vdir */
	vdirfd = open_read(".");

	struct stat vdirsb, tdirsb;

	if (fstat(vdirfd, &vdirsb) == -1)
		method_return_sys_fault(env, "fstat");

	chdir(tdir);

	if (lstat(".", &tdirsb) == -1)
		method_return_sys_fault(env, "lstat");

	if (vdirsb.st_dev != tdirsb.st_dev)
		copy = 1;

	global_env = env;
	nftw(".", handle_file, 50, FTW_MOUNT|FTW_PHYS|FTW_ACTIONRETVAL);
	global_env = NULL;

	close(vdirfd);
	return NULL;
}

static
xmlrpc_value *find_free_xid(xmlrpc_env *env)
{
	LOG_TRACEME

	if (xid != 0)
		return NULL;

	int rc = vxdb_prepare(&dbr,
			"SELECT COUNT(xid),MAX(xid) FROM xid_name_map");

	if (rc == VXDB_OK) {
		vxdb_foreach_step(rc, dbr) {
			int cnt = vxdb_column_int(dbr, 0);

			if (cnt < 1)
				xid = 2;

			else {
				int max = vxdb_column_int(dbr, 1);

				if (max < 65535)
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
	}

	if (!env->fault_occurred && rc != VXDB_DONE)
		method_set_vxdb_fault(env);

	if (xid == 0)
		method_set_faultf(env, MEVXDB,
				"no free context id available: %s", name);

	vxdb_finalize(dbr);
	return NULL;
}

static
xmlrpc_value *build_root_filesystem(xmlrpc_env *env)
{
	LOG_TRACEME

	/* try to get rid of old vdir if this is a rebuild */
	if (xid > 0) {
		const char *oldvdir = vxdb_getvdir(name);

		if (!validate_path(oldvdir))
			method_return_faultf(env, MECONF,
					"invalid old vdir path: %s", oldvdir);

		runlink(oldvdir);
	}

	/* try to runlink vdir first */
	runlink(vdir);

	/* find a free xid */
	find_free_xid(env);
	method_return_if_fault(env);

	/* create new vdir */
	if (mkdirp(vdir, 0755) == -1)
		method_return_sys_fault(env, "mkdirp");

	/* now link/copy the template to the new vdir */
	make_vserver_filesystem(env);

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
				"could not properly sanitize /dev: %s", strerror(errno));

	return NULL;
}

static
xmlrpc_value *create_vxdb_entries(xmlrpc_env *env)
{
	LOG_TRACEME

	char *tconf = NULL;
	asprintf(&tconf, "%s.conf", tdir);

	if (str_isempty(tconf) || !isfile(tconf))
		tconf = "/dev/null";

	cfg_t *tcfg = cfg_init(BUILD_OPTS, CFGF_NOCASE);

	switch (cfg_parse(tcfg, tconf)) {
	case CFG_FILE_ERROR:
		mem_free(tconf);
		method_return_faultf(env, MECONF,
				"could not read template configuration: %s", template);

	case CFG_PARSE_ERROR:
		mem_free(tconf);
		method_return_faultf(env, MECONF,
				"syntax error in template configuration: %s", template);

	default:
		mem_free(tconf);
		break;
	}

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
			"DELETE FROM restart      WHERE xid = %d;"
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

	if (!validate_path(init))
		method_return_faultf(env, MECONF,
				"invalid template configuration for init: %s", init);

	if (!validate_path(halt))
		method_return_faultf(env, MECONF,
				"invalid template configuration for halt: %s", halt);

	if (!validate_path(reboot))
		method_return_faultf(env, MECONF,
				"invalid template configuration for reboot: %s", reboot);

	timeout = timeout < 0 ? 0 : timeout;

	stralloc_catf(sa,
			"INSERT INTO init (xid, init, halt, reboot, timeout) "
			"VALUES (%d, '%s', '%s', '%s', %d);",
			xid, init, halt, reboot, timeout);

	int i;

	/* get mounts */
	int mount_size = cfg_size(tcfg, "mount");

	if (mount_size < 1) {
		stralloc_catf(sa,
				"INSERT INTO mount (xid, src, dst, type, opts) "
				"VALUES (%d, 'none', '/proc', 'proc', 'defaults');",
				xid);

		stralloc_catf(sa,
				"INSERT INTO mount (xid, src, dst, type, opts) "
				"VALUES (%d, 'none', '/dev/pts', 'devpts', 'defaults');",
				xid);
	}

	else for (i = 0; i < mount_size; i++) {
		cfg_t *cfg_mount = cfg_getnsec(tcfg, "mount", i);

		const char *mdst  = cfg_title(cfg_mount);
		const char *msrc  = cfg_getstr(cfg_mount, "src");
		const char *mtype = cfg_getstr(cfg_mount, "type");
		const char *mopts = cfg_getstr(cfg_mount, "opts");

		if (!validate_path(mdst))
			method_return_faultf(env, MECONF,
				"invalid template configuration for mount destination: %s",
				mdst);

		if (!validate_path(msrc) && !str_equal(msrc, "none"))
			method_return_faultf(env, MECONF,
				"invalid template configuration for mount source: %s",
				msrc);

		if (str_isempty(mtype) || !str_isalnum(mtype))
			method_return_faultf(env, MECONF,
				"invalid template configuration for mount type: %s",
				mtype);

		if (str_isempty(mopts) || !str_isascii(mopts))
			method_return_faultf(env, MECONF,
				"invalid template configuration for mount options: %s",
				mopts);

		stralloc_catf(sa,
			"INSERT INTO mount (xid, src, dst, type, opts) "
			"VALUES (%d, '%s', '%s', '%s', '%s');",
			xid, msrc, mdst, mtype, mopts);
	}

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

	/* insert vdir */
	stralloc_catf(sa,
			"INSERT INTO vdir (xid, vdir) VALUES (%d, '%s');",
			xid, vdir);

	/* insert name */
	stralloc_catf(sa,
			"INSERT INTO xid_name_map (xid, name) VALUES (%d, '%s');",
			xid, name);

	/* commit transaction */
	stralloc_cats(sa, "COMMIT TRANSACTION;");

	char *sql = stralloc_finalize(sa);

	if (vxdb_exec(sql) != VXDB_OK) {
		method_set_vxdb_fault(env);
		runlink(vdir);
	}

	return NULL;
}

static
const char *template_description(const char *tbasedir, const char *name)
{
	char *tconf = NULL;
	asprintf(&tconf, "%s/%s.conf", tbasedir, name);

	if (str_isempty(tconf) || !isfile(tconf))
		return "(none)";

	cfg_t *tcfg = cfg_init(BUILD_OPTS, CFGF_NOCASE);

	switch (cfg_parse(tcfg, tconf)) {
	case CFG_FILE_ERROR:
	case CFG_PARSE_ERROR:
		mem_free(tconf);
		return "(parse error in configuration)";

	default:
		mem_free(tconf);
		break;
	}

	const char *description = cfg_getstr(tcfg, "description");

	return str_isempty(description) ? "(none)" : description;
}

/* vx.templates([string name]) */
xmlrpc_value *m_vx_templates(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;

	params = method_init(env, p, c, VCD_CAP_CREATE, 0);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,*}",
			"name", &name);
	method_return_if_fault(env);

	int curfd = open_read(".");

	/* get template dir */
	const char *tbasedir = cfg_getstr(cfg, "templatedir");

	if (chdir(tbasedir) == -1)
		method_return_sys_faultf(env, "chdir(%s)", tbasedir);

	DIR *tfd = opendir(".");

	if (tfd == NULL)
		method_return_sys_faultf(env, "opendir(%s)", tbasedir);

	struct dirent *dent;
	xmlrpc_value *response = xmlrpc_array_new(env);

	/* list all templates */
	while ((dent = readdir(tfd))) {
		if (str_equal(dent->d_name, ".") || str_equal(dent->d_name, "..") ||
				(!str_isempty(name) && !str_equal(name, dent->d_name)))
			continue;

		if (isdir(dent->d_name)) {
			const char *description = template_description(tbasedir,
					dent->d_name);

			xmlrpc_array_append_item(env, response, xmlrpc_build_value(env,
					"{s:s,s:s}",
					"name", dent->d_name,
					"description", description));
		}
	}

	closedir(tfd);
	fchdir(curfd);
	close(curfd);

	return response;
}

/* vx.create(string name, string template, bool force, bool copy[,
 *           string vdir]) */
xmlrpc_value *m_vx_create(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	char *user;
	xmlrpc_value *params;

	method_init(env, p, c, VCD_CAP_CREATE, M_LOCK);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, p,
			"({s:s,*}V)",
			"username", &user,
			&params);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
			"{s:s,s:s,s:b,s:b,s:s,*}",
			"name", &name,
			"template", &template,
			"force", &force,
			"copy", &copy,
			"vdir", &vdir);
	method_return_if_fault(env);

	/* get template dir */
	if (str_isempty(template) || !str_isgraph(template))
		method_return_faultf(env, MEINVAL,
				"invalid template name: %s", template);

	const char *tbasedir = cfg_getstr(cfg, "templatedir");

	if (!validate_path(tbasedir) || !isdir(tbasedir))
		method_return_faultf(env, MECONF,
				"tbasedir invalid path or does not exist: %s", tbasedir);

	if (!(tdir = str_path_concat(tbasedir, template)))
		method_return_faultf(env, MEINVAL,
				"invalid template path: %s/%s", tbasedir, template);

	if (!isdir(tdir))
		method_return_faultf(env, MEINVAL,
				"template does not exist: %s", template);

	/* get old xid for rebuild */
	if ((xid = vxdb_getxid(name))) {
		if (auth_isowner(user, name)) {
			if (!force)
				method_return_fault(env, MEEXIST);
			else if (vx_info(xid, NULL) == 0)
				method_return_fault(env, MERUNNING);
		}

		else
			method_return_fault(env, MENOVPS);
	}

	/* only admins can build new ones */
	else if (!auth_isadmin(user))
		method_return_fault(env, MEPERM);

	/* check vdir */
	if (str_isempty(vdir) || !auth_isowner(user, name))
		vdir = vxdb_getvdir(name);

	if (!validate_path(vdir))
		method_return_faultf(env, MEINVAL,
				"invalid vdir path: %s", vdir);

	if (str_len(vdir) > 31)
		method_return_faultf(env, MEINVAL,
				"vdir path too long: %s", vdir);

	if (!force && ispath(vdir))
		method_return_faultf(env, MEEXIST,
				"vdir path already exists: %s", vdir);

	/* build the vserver root fs from the template */
	build_root_filesystem(env);
	method_return_if_fault(env);

	/* create the needed vxdb entries */
	create_vxdb_entries(env);
	method_return_if_fault(env);

	return xmlrpc_nil_new(env);
}
