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

#include "pathconfig.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>

#include "lucid.h"
#include "confuse.h"
#include "xmlrpc.h"

#include "auth.h"
#include "log.h"
#include "vxdb.h"

static
cfg_opt_t dlimit_VXDB[] = {
	CFG_INT("inodes",   0, CFGF_NONE),
	CFG_INT("space",    0, CFGF_NONE),
	CFG_INT("reserved", 5, CFGF_NONE),
	CFG_END()
};

static
cfg_opt_t init_VXDB[] = {
	CFG_STR("method",         "plain", CFGF_NONE),
	CFG_INT("timeout",        30,      CFGF_NONE),
	CFG_STR("runlevel-start", "3",     CFGF_NONE),
	CFG_STR("runlevel-stop",  "0",     CFGF_NONE),
	CFG_END()
};

static
cfg_opt_t nx_addr_VXDB[] = {
	CFG_STR("ip",        NULL, CFGF_NONE),
	CFG_STR("netmask",   NULL, CFGF_NONE),
	CFG_STR("broadcast", NULL, CFGF_NONE),
	CFG_END()
};

static
cfg_opt_t nx_VXDB[] = {
	CFG_STR("flags", NULL,         CFGF_NONE),
	CFG_SEC("addr",  nx_addr_VXDB, CFGF_MULTI),
	CFG_END()
};

static
cfg_opt_t rlimit_VXDB[] = {
	CFG_INT("min",  0, CFGF_NONE),
	CFG_INT("soft", 0, CFGF_NONE),
	CFG_INT("max",  0, CFGF_NONE),
	CFG_END()
};

static
cfg_opt_t sched_VXDB[] = {
	CFG_INT("fillrate",  0, CFGF_NONE),
	CFG_INT("fillrate2", 0, CFGF_NONE),
	CFG_INT("interval",  0, CFGF_NONE),
	CFG_INT("interval2", 0, CFGF_NONE),
	CFG_INT("priobias",  0, CFGF_NONE),
	CFG_INT("tokens",    0, CFGF_NONE),
	CFG_INT("tokensmin", 0, CFGF_NONE),
	CFG_INT("tokensmax", 0, CFGF_NONE),
	CFG_END()
};

static
cfg_opt_t uts_VXDB[] = {
	CFG_STR("domainname", NULL, CFGF_NONE),
	CFG_STR("machine",    NULL, CFGF_NONE),
	CFG_STR("nodename",   NULL, CFGF_NONE),
	CFG_STR("release",    NULL, CFGF_NONE),
	CFG_STR("sysname",    NULL, CFGF_NONE),
	CFG_STR("version",    NULL, CFGF_NONE),
	CFG_END()
};

static
cfg_opt_t vx_VXDB[] = {
	CFG_STR("bcaps",  NULL, CFGF_NONE),
	CFG_STR("ccaps",  NULL, CFGF_NONE),
	CFG_STR("flags",  NULL, CFGF_NONE),
	CFG_STR("fstab",  NULL, CFGF_NONE),
	CFG_STR("mtab",   NULL, CFGF_NONE),
	CFG_INT("nice",   0,    CFGF_NONE),
	CFG_STR("shell",  NULL, CFGF_NONE),
	CFG_STR("pflags", NULL, CFGF_NONE),
	CFG_END()
};

static
cfg_opt_t VXDB[] = {
	CFG_SEC("dlimit", dlimit_VXDB, CFGF_MULTI | CFGF_TITLE),
	CFG_SEC("init",   init_VXDB,   CFGF_NONE),
	CFG_SEC("nx",     nx_VXDB,     CFGF_NONE),
	CFG_SEC("rlimit", rlimit_VXDB, CFGF_MULTI | CFGF_TITLE),
	CFG_SEC("sched",  sched_VXDB,  CFGF_NONE),
	CFG_SEC("uts",    uts_VXDB,    CFGF_NONE),
	CFG_SEC("vx",     vx_VXDB,     CFGF_NONE),
	CFG_END()
};

cfg_t *vxdb_open(char *name)
{
	cfg_t *cfg;
	char cfg_file[PATH_MAX];
	
	cfg = cfg_init(VXDB, CFGF_NOCASE);
	
	snprintf(cfg_file, PATH_MAX, "%s/vxdb/%s", __LOCALSTATEDIR, name);
	
	switch (cfg_parse(cfg, cfg_file)) {
	case CFG_FILE_ERROR:
		cfg_free(cfg);
		return errno = ENOENT, NULL;
	
	case CFG_PARSE_ERROR:
		cfg_free(cfg);
		return errno = EINVAL, NULL;
	}
	
	return cfg;
}

void vxdb_close(cfg_t *cfg)
{
	cfg_free(cfg);
}

int vxdb_closewrite(cfg_t *cfg)
{
	FILE *fp = fopen(cfg->filename, "w");
	
	if (!fp)
		return -1;
	
	cfg_print(cfg, fp);
	cfg_free(cfg);
	
	return 0;
}

cfg_opt_t *vxdb_lookup(cfg_t *cfg, char *key, char *title)
{
	cfg_t *sec;
	cfg_opt_t *opt;
	
	char *p1 = strdup(key);
	char *p2 = strchr(p1, '.');
	*p2++ = '\0';
	
	if (title)
		sec = cfg_gettsec(cfg, p1, title);
	else
		sec = cfg_getsec(cfg, p1);
	
	if (sec == NULL || (opt = cfg_getopt(sec, p2)) == NULL)
		return errno = ENOENT, NULL;
	
	return opt;
}

int vxdb_addsec(cfg_t *cfg, char *key, char *title)
{
	int i;
	char *p1, *p2;
	cfg_opt_t *opt;
	
	if (!cfg || !key || !title)
		return errno = EINVAL, -1;
	
	p1 = strdup(key);
	p2 = strchr(p1, '.');
	*p2++ = '\0';
	
	opt = cfg_getopt(cfg, p1);
	
	free(p1);
	
	if (!opt)
		return errno = EINVAL, -1;
	
	if (cfg_setopt(cfg, opt, title) == NULL)
		return errno = EINVAL, -1;
	
	return 0;
}

int vxdb_capable(XMLRPC_VALUE auth, char *name, char *key, int write)
{
	SDBM *db;
	DATUM k, v;
	char *buf, *p;
	int rc = 0;
	
	char *username = (char *) XMLRPC_VectorGetStringWithID(auth, "username");
	
	if (!username || !auth_vxowner(auth, name))
		return 0;
	
	if (auth_isadmin(auth))
		return 1;
	
	if (write == 0)
		db = sdbm_open(__LOCALSTATEDIR "/vxdb/acl_read", O_RDONLY, 0);
	else
		db = sdbm_open(__LOCALSTATEDIR "/vxdb/acl_write", O_RDONLY, 0);
	
	if (!db)
		return log_warn("sdbm_open: %s", strerror(errno)), 0;
	
	k.dptr  = username;
	k.dsize = strlen(k.dptr);
	
	v = sdbm_fetch(db, k);
	
	sdbm_close(db);
	
	if (v.dsize > 0) {
		buf = strndup(v.dptr, v.dsize);
		
		while ((p = strsep(&buf, ",")) != NULL) {
			if (strcmp(key, p) == 0) {
				rc = 1;
				break;
			}
		}
		
		free(buf);
	}
	
	return rc;
}
