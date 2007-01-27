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
#include <fcntl.h>
#include <confuse.h>
#include <vserver.h>
#include <sys/stat.h>
#include <lucid/chroot.h>
#include <lucid/flist.h>
#include <lucid/misc.h>
#include <lucid/open.h>
#include <lucid/str.h>
#include <lucid/stralloc.h>
#include <libtar.h>

#include "auth.h"
#include "cfg.h"
#include "lists.h"
#include <lucid/log.h>
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

#define DEFAULT_BCAPS "{ LINUX_IMMUTABLE, NET_BROADCAST, NET_ADMIN, NET_RAW, " \
                      "IPC_LOCK, IPC_OWNER, SYS_MODULE, SYS_RAWIO, " \
                      "SYS_PACCT, SYS_ADMIN, SYS_NICE, SYS_RESOURCE, " \
                      "SYS_TIME, MKNOD, AUDIT_CONTROL }"

#define DEFAULT_CCAPS "{ SET_UTSNAME, RAW_ICMP }"

#define DEFAULT_FLAGS "{ VIRT_MEM, VIRT_UPTIME, VIRT_CPU, " \
                      "VIRT_LOAD, HIDE_NETIF }"

static cfg_opt_t init_OPTS[] = {
	CFG_STR("init",    "/sbin/init",   CFGF_NONE),
	CFG_STR("halt",    "/sbin/halt",   CFGF_NONE),
	CFG_STR("reboot",  "/sbin/reboot", CFGF_NONE),
	CFG_END()
};

static cfg_opt_t BUILD_OPTS[] = {
	CFG_STR("description", NULL, CFGF_NONE),

	CFG_SEC("init", init_OPTS, CFGF_NONE),

	CFG_STR_LIST("vx_bcaps",  DEFAULT_BCAPS, CFGF_NONE),
	CFG_STR_LIST("vx_ccaps",  DEFAULT_CCAPS, CFGF_NONE),
	CFG_STR_LIST("vx_flags",  DEFAULT_FLAGS, CFGF_NONE),
	CFG_END()
};

static xid_t xid = 0;
static char *name = NULL;
static int rebuild = 0;

static
xmlrpc_value *build_root_filesystem(xmlrpc_env *env, const char *template)
{
	LOG_TRACEME

	const char *datadir, *vdir;
	char archive[PATH_MAX];
	struct stat sb;
	TAR *t;

	datadir = cfg_getstr(cfg, "datadir");
	snprintf(archive, PATH_MAX, "%s/templates/%s.tar", datadir, template);

	if (!(vdir = vxdb_getvdir(name)))
		method_return_faultf(env, MECONF, "invalid vdir: %s", vdir);

	/* TODO: detect mounts */
	if (rebuild && runlink(vdir) == -1)
		method_return_faultf(env, MESYS, "runlink: %s", strerror(errno));

	if (lstat(vdir, &sb) == -1) {
		if (errno != ENOENT)
			method_return_faultf(env, MESYS, "lstat: %s", strerror(errno));
	}

	else
		method_return_fault(env, MEEXIST);

	if (mkdirp(vdir, 0755) == -1 || chroot_secure_chdir(vdir, "/") == -1)
		method_return_faultf(env, MESYS, "secure_chdir: %s", strerror(errno));

	if (strcmp(template, "skeleton") != 0) {
		if (tar_open(&t, archive, NULL, O_RDONLY, 0, 0) == -1)
			method_return_faultf(env, MESYS, "tar_open: %s", strerror(errno));

		if (tar_extract_all(t, ".") != 0)
			method_return_faultf(env, MESYS, "tar_extract_all: %s", strerror(errno));

		tar_close(t);
	}

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
		method_return_faultf(env, MESYS, "could not sanitize /dev: %s", strerror(errno));

	return NULL;
}

static
xid_t find_free_xid()
{
	LOG_TRACEME

	int i;
	char *name;

	for (i = 2; i < 65535; i++)
		if (!(name = vxdb_getname(i)))
			return i;

	return 0;
}

static
xmlrpc_value *create_vxdb_entries(xmlrpc_env *env, const char *template)
{
	LOG_TRACEME

	int rc, i;
	vxdb_result *dbr;
	stralloc_t sa;
	cfg_t *tcfg;

	int max, cnt;

	cfg_t *init_cfg;
	const char *init, *halt, *reboot;

	int vx_bcaps_size, vx_ccaps_size, vx_flags_size;
	const char *bcap, *ccap, *flag;

	char *datadir, templateconf[PATH_MAX];
	struct stat sb;

	char *sql;

	/* load configuration */
	datadir = cfg_getstr(cfg, "datadir");
	snprintf(templateconf, PATH_MAX, "%s/templates/%s.conf", datadir, template);

	if (lstat(templateconf, &sb) == -1) {
		if (errno != ENOENT)
			method_return_faultf(env, MESYS, "lstat: %s", strerror(errno));

		else
			strncpy(templateconf, "/dev/null", PATH_MAX);
	}

	tcfg = cfg_init(BUILD_OPTS, CFGF_NOCASE);

	switch (cfg_parse(tcfg, templateconf)) {
	case CFG_FILE_ERROR:
		method_return_faultf(env, MECONF, "no template configuration found for '%s'", template);

	case CFG_PARSE_ERROR:
		method_return_faultf(env, MECONF, "%s", "syntax error in template configuration");

	default:
		break;
	}

	/* find a free xid */
	if (!xid) {
		rc = vxdb_prepare(&dbr, "SELECT COUNT(xid),MAX(xid) FROM xid_name_map");

		if (rc || vxdb_step(dbr) < 1)
			method_set_fault(env, MEVXDB);

		else {
			cnt = sqlite3_column_int(dbr, 0);
			max = sqlite3_column_int(dbr, 1);

			if (max < 2)
				xid = 2;

			else if (max < 65535)
				xid = max + 1;

			else if (cnt < 65535)
				xid = find_free_xid();

			else
				method_set_faultf(env, MEVXDB, "%s", "no free context id available");
		}

		sqlite3_finalize(dbr);
	}

	method_return_if_fault(env);

	/* assemble SQL query */
	stralloc_init(&sa);
	stralloc_cats(&sa, "BEGIN EXCLUSIVE TRANSACTION;");

	init_cfg = cfg_getsec(tcfg, "init");

	if (init_cfg) {
		init    = cfg_getstr(init_cfg, "init");
		halt    = cfg_getstr(init_cfg, "halt");
		reboot  = cfg_getstr(init_cfg, "reboot");

		if (str_isempty(init))
			init = "/sbin/init";

		if (str_isempty(halt))
			halt = "/sbin/halt";

		if (str_isempty(reboot))
			reboot = "/sbin/reboot";

		stralloc_catf(&sa,
			"INSERT OR REPLACE INTO init (xid, init, halt, reboot) "
			"VALUES (%d, '%s', '%s', '%s');",
			xid, init, halt, reboot);
	}

	if (rebuild)
		goto commit;

	vx_bcaps_size = cfg_size(tcfg, "vx_bcaps");

	for (i = 0; i < vx_bcaps_size; i++) {
		bcap = cfg_getnstr(tcfg, "vx_bcaps", i);

		if (!flist64_getval(bcaps_list, bcap))
			method_return_faultf(env, MECONF, "invalid bcap: %s", bcap);

		stralloc_catf(&sa,
			"INSERT OR REPLACE INTO vx_bcaps (xid, bcap) "
			"VALUES (%d, '%s');",
			xid, bcap);
	}

	vx_ccaps_size = cfg_size(tcfg, "vx_ccaps");

	for (i = 0; i < vx_ccaps_size; i++) {
		ccap = cfg_getnstr(tcfg, "vx_ccaps", i);

		if (!flist64_getval(ccaps_list, ccap))
			method_return_faultf(env, MECONF, "invalid ccap: %s", ccap);

		stralloc_catf(&sa,
			"INSERT OR REPLACE INTO vx_ccaps (xid, ccap) "
			"VALUES (%d, '%s');",
			xid, ccap);
	}

	vx_flags_size = cfg_size(tcfg, "vx_flags");

	for (i = 0; i < vx_flags_size; i++) {
		flag = cfg_getnstr(tcfg, "vx_flags", i);

		if (!flist64_getval(cflags_list, flag))
			method_return_faultf(env, MECONF, "invalid cflag: %s", flag);

		stralloc_catf(&sa,
			"INSERT OR REPLACE INTO vx_flags (xid, flag) "
			"VALUES (%d, '%s');",
			xid, flag);
	}

	stralloc_catf(&sa,
		"INSERT OR REPLACE INTO xid_name_map (xid, name) VALUES (%d, '%s');",
		xid, name);

commit:
	stralloc_cats(&sa, "COMMIT TRANSACTION;");
	sql = strndup(sa.s, sa.len);
	stralloc_free(&sa);

	rc = vxdb_exec(sql);

	if (rc)
		method_set_fault(env, MEVXDB);

	return NULL;
}

/* vx.create(string name, string template, bool rebuild) */
xmlrpc_value *m_vx_create(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *params;
	char *user, *template;

	method_init(env, p, c, VCD_CAP_CREATE, M_LOCK);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, p,
		"({s:s,*}V)",
		"username", &user,
		&params);
	method_return_if_fault(env);

	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,s:b,*}",
		"name", &name,
		"template", &template,
		"rebuild", &rebuild);
	method_return_if_fault(env);

	if (!validate_name(name) || str_isempty(template) || !str_isgraph(template))
		method_return_fault(env, MEINVAL);

	if ((xid = vxdb_getxid(name))) {
		if (auth_isadmin(user) || auth_isowner(user, name)) {
			if (!rebuild)
				method_return_fault(env, MEEXIST);
			else if (vx_info(xid, NULL) == 0)
				method_return_fault(env, MERUNNING);
		}

		else
			method_return_fault(env, MEPERM);
	}

	else {
		xid = 0;
		rebuild = 0;
	}

	build_root_filesystem(env, template);
	method_return_if_fault(env);

	create_vxdb_entries(env, template);
	method_return_if_fault(env);

	return xmlrpc_nil_new(env);
}
