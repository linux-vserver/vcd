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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <confuse.h>
#include <vserver.h>
#include <archive.h>
#include <archive_entry.h>
#include <sys/stat.h>

#include "lucid.h"
#include "xmlrpc.h"

#include "auth.h"
#include "cfg.h"
#include "lists.h"
#include "log.h"
#include "methods.h"
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

static cfg_opt_t init_mount_OPTS[] = {
	CFG_STR("spec",    NULL, CFGF_NONE),
	CFG_STR("vfstype", NULL, CFGF_NONE),
	CFG_STR("mntops",  NULL, CFGF_NONE),
	CFG_END()
};

static cfg_opt_t BUILD_OPTS[] = {
	CFG_STR("archive",     NULL, CFGF_NONE),
	CFG_STR("description", NULL, CFGF_NONE),
	
	CFG_SEC("init_method", init_method_OPTS, CFGF_NONE),
	CFG_SEC("init_mount",  init_mount_OPTS,  CFGF_MULTI|CFGF_TITLE),
	
	CFG_STR_LIST("vx_bcaps",  DEFAULT_BCAPS, CFGF_NONE),
	CFG_STR_LIST("vx_ccaps",  DEFAULT_CCAPS, CFGF_NONE),
	CFG_STR_LIST("vx_flags",  DEFAULT_FLAGS, CFGF_NONE),
	CFG_END()
};

static
int template_extract(const char *filename)
{
	struct archive *a;
	struct archive_entry *entry;
	int r, flags = 0;
	
	flags |= ARCHIVE_EXTRACT_OWNER;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;
	
	a = archive_read_new();
	archive_read_support_compression_bzip2(a);
	archive_read_support_compression_gzip(a);
	archive_read_support_format_tar(a);
	
	if ((r = archive_read_open_file(a, filename, 10240)))
		return errno = MECONF, -1;
	
	while (1) {
		r = archive_read_next_header(a, &entry);
		
		if (r == ARCHIVE_EOF)
			break;
		
		if (r != ARCHIVE_OK)
			return errno = MECONF, -1;
		
		if (archive_read_extract(a, entry, flags))
			return errno = MECONF, -1;
	}
	
	archive_read_close(a);
	archive_read_finish(a);
	
	return 0;
}

/* vx.create(string name, int xid, string template[, int rebuild]) */
XMLRPC_VALUE m_vx_create(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	int i, errno_orig;
	char *buf;
	
	XMLRPC_VALUE params = method_get_params(r);
	
	char *name     = XMLRPC_VectorGetStringWithID(params, "name");
	char *template = XMLRPC_VectorGetStringWithID(params, "template");
	xid_t xid      = XMLRPC_VectorGetIntWithID(params, "xid");
	int rebuild    = XMLRPC_VectorGetIntWithID(params, "rebuild");
	
	if (!name || !*name || !template || !*template || xid < 2)
		return method_error(MEREQ);
	
	xid_t xid_buf;
	
	if (vxdb_getxid(name, &xid_buf) == 0) {
		if (xid == xid_buf) {
			if (auth_isadmin(r) || auth_isowner(r)) {
				if (!rebuild)
					return method_error(MEEXIST);
				else if (vx_get_info(xid, NULL) == 0)
					return method_error(MERUNNING);
			}
			
			else
				return method_error(MEPERM);
		}
		
		else if (auth_isadmin(r))
			return method_error(MEEXIST);
		
		else
			return method_error(MEPERM);
	}
	
	else if (!auth_isadmin(r))
		return method_error(MEPERM);
	
	else
		rebuild = 0;
	
	char *vdirbase = cfg_getstr(cfg, "vserver-dir");
	
	if (!vdirbase || !*vdirbase)
		return method_error(MECONF);
	
	char vdir[PATH_MAX];
	snprintf(vdir, PATH_MAX, "%s/%s", vdirbase, name);
	
	char *templatedir = cfg_getstr(cfg, "template-dir");
	
	if (!templatedir || !*templatedir)
		return method_error(MENOENT);
	
	char templateconf[PATH_MAX];
	snprintf(templateconf, PATH_MAX, "%s/%s.conf", templatedir, template);
	
	cfg_t *build_cfg = cfg_init(BUILD_OPTS, CFGF_NOCASE);
	
	switch (cfg_parse(build_cfg, templateconf)) {
	case CFG_FILE_ERROR:
		return method_error(MENOENT);
	
	case CFG_PARSE_ERROR:
		return method_error(MECONF);
	
	default:
		break;
	}
	
	char *archivefile = cfg_getstr(build_cfg, "archive");
	
	if (!archivefile || !*archivefile)
		return method_error(MECONF);
	
	char templatearchive[PATH_MAX];
	snprintf(templatearchive, PATH_MAX, "%s/%s", templatedir, archivefile);
	
	if (rebuild && runlink(vdir) == -1)
		return method_error(MESYS);
	
	struct stat sb;
	
	if (lstat(vdir, &sb) == -1) {
		if (errno != ENOENT)
			return method_error(MESYS);
	}
	
	else
		return method_error(MEEXIST);
	
	if (mkdirp(vdir, 0755) == -1 || chroot_secure_chdir(vdir, ".") == -1)
		return method_error(MESYS);
	
	if (template_extract(templatearchive) == -1)
		return method_error(errno);
	
	if (runlink("dev") == -1 ||
	    mkdir("dev", 0755) == -1 ||
	    mknod("dev/null",    0666 | S_IFCHR, makedev(1,3)) == -1 ||
	    mknod("dev/zero",    0666 | S_IFCHR, makedev(1,5)) == -1 ||
	    mknod("dev/full",    0666 | S_IFCHR, makedev(1,7)) == -1 ||
	    mknod("dev/random",  0644 | S_IFCHR, makedev(1,8)) == -1 ||
	    mknod("dev/urandom", 0644 | S_IFCHR, makedev(1,9)) == -1 ||
	    mknod("dev/tty",     0666 | S_IFCHR, makedev(5,0)) == -1 ||
	    mknod("dev/ptmx",    0666 | S_IFCHR, makedev(5,2)) == -1)
		return method_error(MESYS);
	
	if (!dbi_conn_queryf(vxdb, "BEGIN TRANSACTION"))
		return method_error(MEVXDB);
	
	cfg_t *init_method_cfg = cfg_getsec(build_cfg, "init_method");
	
	if (init_method_cfg) {
		char *method  = cfg_getstr(init_method_cfg, "method");
		char *start   = cfg_getstr(init_method_cfg, "start");
		char *stop    = cfg_getstr(init_method_cfg, "stop");
		int   timeout = cfg_getint(init_method_cfg, "timeout");
		
		if (!method || !*method)
			method = "init";
		
		if (!start || !*start)
			start = "";
		
		if (!stop || !*stop)
			stop = "";
		
		if (timeout < 0)
			timeout = 0;
		
		if (!dbi_conn_queryf(vxdb,
			"INSERT OR REPLACE INTO init_method (xid, method, start, stop, timeout) "
			"VALUES (%d, '%s', '%s', '%s', %d)",
			xid, method, start, stop, timeout)) goto rolledback;
	}
	
	int init_mount_size = cfg_size(build_cfg, "init_mount");
	
	for (i = 0; i < init_mount_size; i++) {
		cfg_t *init_mount_cfg = cfg_getnsec(build_cfg, "init_mount", i);
		
		char *file    = (char *) cfg_title(init_mount_cfg);
		char *spec    = cfg_getstr(init_mount_cfg, "spec");
		char *vfstype = cfg_getstr(init_mount_cfg, "vfstype");
		char *mntops  = cfg_getstr(init_mount_cfg, "mntops");
		
		if (!file || !*file || !spec || !*spec) {
			errno = MECONF;
			goto rollback;
		}
		
		if (!mntops || !*mntops)
			mntops = "defaults";
		
		if (!vfstype || !*vfstype)
			vfstype = "auto";
		
		if (!dbi_conn_queryf(vxdb,
			"INSERT OR REPLACE INTO init_mount (xid, file, spec, vfstype, mntops) "
			"VALUES (%d, '%s', '%s', '%s', '%s')",
			xid, file, spec, vfstype, mntops)) goto rolledback;
		
		if (chroot_mkdirp(vdir, file, 0755) == -1) {
			errno = MESYS;
			goto rollback;
		}
	}
	
	if (rebuild)
		goto out;
	
	int vx_bcaps_size = cfg_size(build_cfg, "vx_bcaps");
	
	for (i = 0; i < vx_bcaps_size; i++) {
		char *bcap = cfg_getnstr(build_cfg, "vx_bcaps", i);
		
		if (flist64_getval(bcaps_list, bcap, NULL) == -1) {
			errno = MECONF;
			goto rollback;
		}
		
		if (!dbi_conn_queryf(vxdb,
			"INSERT OR ROLLBACK INTO vx_bcaps (xid, bcap) "
			"VALUES (%d, '%s')",
			xid, bcap)) goto rolledback;
	}
	
	int vx_ccaps_size = cfg_size(build_cfg, "vx_ccaps");
	
	for (i = 0; i < vx_ccaps_size; i++) {
		char *ccap = cfg_getnstr(build_cfg, "vx_ccaps", i);
		
		if (flist64_getval(ccaps_list, ccap, NULL) == -1) {
			errno = MECONF;
			goto rollback;
		}
		
		if (!dbi_conn_queryf(vxdb,
			"INSERT OR ROLLBACK INTO vx_ccaps (xid, ccap) "
			"VALUES (%d, '%s')",
			xid, ccap)) goto rolledback;
	}
	
	int vx_flags_size = cfg_size(build_cfg, "vx_flags");
	
	for (i = 0; i < vx_flags_size; i++) {
		char *flag = cfg_getnstr(build_cfg, "vx_flags", i);
		
		if (flist64_getval(cflags_list, flag, NULL) == -1) {
			errno = MECONF;
			goto rollback;
		}
		
		if (!dbi_conn_queryf(vxdb,
			"INSERT OR ROLLBACK INTO vx_flags (xid, flag) "
			"VALUES (%d, '%s')",
			xid, flag)) goto rolledback;
	}
	
	if (!dbi_conn_queryf(vxdb,
		"INSERT OR ROLLBACK INTO xid_name_map (xid, name) VALUES (%d, '%s')",
		xid, name)) goto rolledback;
	
	int uid;
	
	if (!auth_isadmin(r) && (uid = auth_getuid(r)) > 0) {
		if (!dbi_conn_queryf(vxdb,
			"INSERT OR ROLLBACK INTO xid_uid_map (xid, uid) VALUES (%d, %d)",
			xid, uid)) goto rolledback;
	}
	
out:
	if (dbi_conn_queryf(vxdb, "COMMIT TRANSACTION"))
		return params;
	/* fall through just in case */
	
rolledback:
	dbi_conn_queryf(vxdb, "ROLLBACK TRANSACTION");
	return method_error(MEVXDB);
	
rollback:
	errno_orig = errno;
	dbi_conn_queryf(vxdb, "ROLLBACK TRANSACTION");
	return method_error(errno_orig);
}
