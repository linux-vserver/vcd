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
#include <confuse.h>
#include <vserver.h>
#include <sys/stat.h>

#include "archive.h"
#include "archive_entry.h"
#include "lucid.h"

#include "auth.h"
#include "cfg.h"
#include "lists.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

#define DEFAULT_BCAPS "{ LINUX_IMMUTABLE, NET_BROADCAST, NET_ADMIN, NET_RAW, " \
                      "IPC_LOCK, IPC_OWNER, SYS_MODULE, SYS_RAWIO, " \
                      "SYS_PACCT, SYS_ADMIN, SYS_NICE, SYS_RESOURCE, " \
                      "SYS_TIME, MKNOD, AUDIT_CONTROL }"

#define DEFAULT_CCAPS "{ SET_UTSNAME, RAW_ICMP }"

#define DEFAULT_FLAGS "{ INFO_HIDE, VIRT_MEM, VIRT_UPTIME, VIRT_CPU, " \
                      "VIRT_LOAD, HIDE_NETIF }"

static cfg_opt_t init_method_OPTS[] = {
	CFG_STR("method",  "init", CFGF_NONE),
	CFG_STR("start",   NULL,   CFGF_NONE),
	CFG_STR("stop",    NULL,   CFGF_NONE),
	CFG_INT("timeout", 15,     CFGF_NONE),
	CFG_END()
};

static cfg_opt_t mount_OPTS[] = {
	CFG_STR("spec",    NULL, CFGF_NONE),
	CFG_STR("vfstype", NULL, CFGF_NONE),
	CFG_STR("mntops",  NULL, CFGF_NONE),
	CFG_END()
};

static cfg_opt_t BUILD_OPTS[] = {
	CFG_STR("archive",     NULL, CFGF_NONE),
	CFG_STR("description", NULL, CFGF_NONE),
	
	CFG_SEC("init_method", init_method_OPTS, CFGF_NONE),
	
	CFG_SEC("mount", mount_OPTS, CFGF_MULTI|CFGF_TITLE),
	
	CFG_STR_LIST("vx_bcaps",  DEFAULT_BCAPS, CFGF_NONE),
	CFG_STR_LIST("vx_ccaps",  DEFAULT_CCAPS, CFGF_NONE),
	CFG_STR_LIST("vx_flags",  DEFAULT_FLAGS, CFGF_NONE),
	CFG_END()
};

static xid_t xid = 0;
static char *name = NULL;
static char vdir[PATH_MAX];
static int rebuild = 0;

static
xmlrpc_value *build_root_filesystem(xmlrpc_env *env, int fd)
{
	const char *vserverdir;
	struct stat sb;
	struct archive *a;
	struct archive_entry *entry;
  int rc, flags = 0;
	
	vserverdir = cfg_getstr(cfg, "vserverdir");
	
	snprintf(vdir, PATH_MAX, "%s/%s", vserverdir, name);
	
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
	
	flags |= ARCHIVE_EXTRACT_OWNER;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;
	
	a = archive_read_new();
	archive_read_support_compression_bzip2(a);
	archive_read_support_compression_gzip(a);
	archive_read_support_format_tar(a);
	
	if (archive_read_open_fd(a, fd, 10240))
		method_return_faultf(env, MESYS, "%s", "could not open template archive for reading");
	
	while (1) {
		rc = archive_read_next_header(a, &entry);
		
		if (rc == ARCHIVE_EOF)
			break;
		
		if (rc != ARCHIVE_OK)
			method_return_faultf(env, MESYS, "%s", "could not read template archive");
		
		if (archive_read_extract(a, entry, flags))
			method_return_faultf(env, MESYS, "%s", "could not extract template archive");
	}
	
	archive_read_close(a);
	archive_read_finish(a);
	
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
	int i;
	char *name;
	
	for (i = 2; i < 65535; i++) {
		if (!(name = vxdb_getname(i)))
			return i;
		
		free(name);
	}
	
	return 0;
}

static
xmlrpc_value *create_vxdb_entries(xmlrpc_env *env, cfg_t *tcfg)
{
	stralloc_t sa;
	cfg_t *init_method_cfg, *mount_cfg;
	const char *method, *start, *stop;
	const char *path, *spec, *vfstype, *mntops;
	int i, timeout, mount_size, max, cnt;
	int vx_bcaps_size, vx_ccaps_size, vx_flags_size;
	const char *bcap, *ccap, *flag;
	char *sql;
	dbi_result dbr;
	
	if (!xid) {
		dbr = dbi_conn_queryf(vxdb,
			"SELECT xid FROM xid_name_map ORDER BY xid DESC");
		
		if (!dbr)
			method_return_fault(env, MEVXDB);
		
		cnt = dbi_result_get_numrows(dbr);
		dbi_result_first_row(dbr);
		max = dbi_result_get_int(dbr, "xid");
		
		if (max < 2)
			xid = 2;
		
		else if (max < 65535)
			xid = max + 1;
		
		else if (cnt < 65535)
			xid = find_free_xid();
		
		else
			method_return_faultf(env, MEVXDB, "%s", "no free context id available");
	}
	
	stralloc_init(&sa);
	stralloc_cats(&sa, "BEGIN TRANSACTION;");
	
	init_method_cfg = cfg_getsec(tcfg, "init_method");
	
	if (init_method_cfg) {
		method  = cfg_getstr(init_method_cfg, "method");
		start   = cfg_getstr(init_method_cfg, "start");
		stop    = cfg_getstr(init_method_cfg, "stop");
		timeout = cfg_getint(init_method_cfg, "timeout");
		
		if (str_isempty(method))
			method = "init";
		
		if (str_isempty(start))
			start = "";
		
		if (str_isempty(stop))
			stop = "";
		
		if (timeout < 0)
			timeout = 0;
		
		stralloc_catf(&sa,
			"INSERT OR REPLACE INTO init_method (xid, method, start, stop, timeout) "
			"VALUES (%d, '%s', '%s', '%s', %d);",
			xid, method, start, stop, timeout);
	}
	
	mount_size = cfg_size(tcfg, "mount");
	
	for (i = 0; i < mount_size; i++) {
		mount_cfg = cfg_getnsec(tcfg, "mount", i);
		
		path    = cfg_title(mount_cfg);
		spec    = cfg_getstr(mount_cfg, "spec");
		vfstype = cfg_getstr(mount_cfg, "vfstype");
		mntops  = cfg_getstr(mount_cfg, "mntops");
		
		if (!validate_path(path))
			method_return_faultf(env, MECONF, "invalid mount path: %s", path);
		
		if (str_isempty(mntops))
			mntops = "defaults";
		
		if (str_isempty(vfstype))
			vfstype = "auto";
		
		stralloc_catf(&sa,
			"INSERT OR REPLACE INTO mount (xid, path, spec, vfstype, mntops) "
			"VALUES (%d, '%s', '%s', '%s', '%s');",
			xid, path, spec, vfstype, mntops);
		
		if (chroot_mkdirp(vdir, path, 0755) == -1)
			method_return_faultf(env, MESYS, "chroot_mkdirp: %s", strerror(errno));
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
	
	if (!dbi_conn_queryf(vxdb, sql)) {
		free(sql);
		method_return_fault(env, MEVXDB);
	}
	
	free(sql);
	
	return NULL;
}

/* vx.create(string name, string template, int rebuild) */
xmlrpc_value *m_vx_create(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	char *user, *template, *templateconf, *templatearchive;
	const char *datadir, *archivename;
	cfg_t *tcfg;
	int archivefd;
	
	method_init(env, p, VCD_CAP_CREATE, 0);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, p,
		"({s:s,*}V)",
		"username", &user,
		&params);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,s:s,s:i,*}",
		"name", &name,
		"template", &template,
		"rebuild", &rebuild);
	method_return_if_fault(env);
	
	if (!validate_name(name) || str_isempty(template))
		method_return_fault(env, MEINVAL);
	
	if ((xid = vxdb_getxid(name))) {
		if (auth_isadmin(user) || auth_isowner(user, name)) {
			if (!rebuild)
				method_return_fault(env, MEEXIST);
			else if (vx_get_info(xid, NULL) == 0)
				method_return_fault(env, MERUNNING);
		}
		
		else
			method_return_fault(env, MEPERM);
	}
		
	else {
		xid = 0;
		rebuild = 0;
	}
	
	datadir = cfg_getstr(cfg, "datadir");
	
	asprintf(&templateconf, "%s/templates/%s.conf", datadir, template);
	
	tcfg = cfg_init(BUILD_OPTS, CFGF_NOCASE);
	
	switch (cfg_parse(tcfg, templateconf)) {
	case CFG_FILE_ERROR:
		free(templateconf);
		method_return_faultf(env, MECONF, "no template configuration found for '%s'", template);
	
	case CFG_PARSE_ERROR:
		free(templateconf);
		method_return_faultf(env, MECONF, "%s", "syntax error in template configuration");
	
	default:
		free(templateconf);
		break;
	}
	
	archivename = cfg_getstr(tcfg, "archive");
	
	if (str_isempty(archivename))
		method_return_faultf(env, MECONF, "invalid archive configuration for '%s'", template);
	
	asprintf(&templatearchive, "%s/templates/%s", datadir, archivename);
	
	if ((archivefd = open_read(templatearchive)) == -1) {
		free(templatearchive);
		method_return_faultf(env, MECONF, "could not open template archive: %s", strerror(errno));
	}
	
	free(templatearchive);
	
	build_root_filesystem(env, archivefd);
	method_return_if_fault(env);
	
	create_vxdb_entries(env, tcfg);
	method_return_if_fault(env);
	
	return xmlrpc_nil_new(env);
}
