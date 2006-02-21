/***************************************************************************
 *   Copyright 2005-2006 by the vserver-utils team                         *
 *   See AUTHORS for details                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <libgen.h>
#include <vserver.h>

#include <lucid/mmap.h>
#include <lucid/open.h>

#include "pathconfig.h"
#include "vc.h"

/* configuration map */
vc_cfg_node_t vc_cfg_map[] = {
	{ "init.runlevel",     VC_CFG_STR_T,  "init/runlevel" },
	{ "init.style",        VC_CFG_STR_T,  "init/style" },
	{ "limit.anon",        VC_CFG_LIST_T, "limits/anon" },
	{ "limit.as",          VC_CFG_LIST_T, "limits/as" },
	{ "limit.memlock",     VC_CFG_LIST_T, "limits/memlock" },
	{ "limit.nofile",      VC_CFG_LIST_T, "limits/nofile" },
	{ "limit.nproc",       VC_CFG_LIST_T, "limits/nproc" },
	{ "limit.rss",         VC_CFG_LIST_T, "limits/rss" },
	{ "ns.fstab",          VC_CFG_STR_T,  "ns/fstab" },
	{ "ns.mtab",           VC_CFG_STR_T,  "ns/mtab" },
	{ "ns.root",           VC_CFG_STR_T,  "ns/root" },
	{ "nx.addr",           VC_CFG_LIST_T, "net/addr" },
	{ "nx.flags",          VC_CFG_LIST_T, "net/addr" },
	{ "vhi.domainname",    VC_CFG_STR_T,  "vhi/domainname" },
	{ "vhi.machine",       VC_CFG_STR_T,  "vhi/machine" },
	{ "vhi.nodename",      VC_CFG_STR_T,  "vhi/nodename" },
	{ "vhi.release",       VC_CFG_STR_T,  "vhi/release" },
	{ "vhi.sysname",       VC_CFG_STR_T,  "vhi/sysname" },
	{ "vhi.version",       VC_CFG_STR_T,  "vhi/version" },
	{ "vps.shell",         VC_CFG_STR_T,  "shell" },
	{ "vps.timeout",       VC_CFG_INT_T,  "timeout" },
	{ "vx.bcaps",          VC_CFG_LIST_T, "bcapabilities" },
	{ "vx.ccaps",          VC_CFG_LIST_T, "ccapabilities" },
	{ "vx.flags",          VC_CFG_LIST_T, "flags" },
	{ "vx.id",             VC_CFG_INT_T,  "id" },
	{ "vx.sched",          VC_CFG_LIST_T, "scheduler" },
	{ NULL,                0,             NULL },
};

/* errno buffer */
int errno_orig;

int vc_cfg_get_type(char *key)
{
	int i;
	
	for (i = 0; vc_cfg_map[i].key; i++)
		if (strcasecmp(vc_cfg_map[i].key, key) == 0)
			return vc_cfg_map[i].type;
	
	errno = EINVAL;
	return -1;
}

int vc_cfg_get_file(char *key, char **file)
{
	int i;
	
	for (i = 0; vc_cfg_map[i].key; i++) {
		if (strcmp(vc_cfg_map[i].key, key) == 0) {
			vc_asprintf(file, "%s", vc_cfg_map[i].file);
			return 0;
		}
	}
	
	errno = EINVAL;
	return -1;
}

int vc_cfg_istype(char *key, int type)
{
	int buf = vc_cfg_get_type(key);
	
	if (buf == -1)
		return -1;
	
	if (buf == type)
		return 1;
	
	return 0;
}

int vc_cfg_get_xid(char *name, xid_t *xid)
{
	int buf;
	
	if (vc_cfg_get_int(name, "vx.id", &buf) == -1)
		return -1;
	
	*xid = (xid_t) buf;
	
	return 0;
}

int vc_cfg_get_name(xid_t xid, char **name)
{
	struct vx_vhi_name vhi_name;
	vhi_name.field = VHIN_CONTEXT;
	
	if (vx_get_vhi_name(xid, &vhi_name) == -1)
		return -1;
	
	vc_asprintf(name, "%s", vhi_name.name);
	
	return 0;
}

static
unsigned long _cfg_readkey(char *name, char *key, char **buf)
{
	char *file;
	
	if (vc_cfg_get_file(key, &file) == -1)
		return -1;
	
	char path[PATH_MAX];
	vc_snprintf(path, PATH_MAX, "%s/%s/%s", __PKGCONFDIR, name, file);
	
	free(file);
	
	size_t len;
	char *buf2;
	buf2 = mmap_private(path, &len);
	
	if (buf2 == NULL)
		return 0;
	
	if (buf2[len-1] == '\n')
		buf2[len-1] = '\0';
	
	*buf = buf2;
	
	return len;
}

static
int _cfg_writekey(char *name, char *key, char *value)
{
	int fd;
	char *file;
	
	if (vc_cfg_get_file(key, &file) == -1)
		return -1;
	
	struct stat sb;
	
	if (lstat(__PKGCONFDIR, &sb) == -1)
		goto error;
	
	if (!S_ISDIR(sb.st_mode)) {
		errno = ENOTDIR;
		goto error;
	}
	
	char path[PATH_MAX];
	vc_snprintf(path, PATH_MAX, "%s/%s", __PKGCONFDIR, name);
	
	if (lstat(path, &sb) == -1) {
		if (errno == ENOENT) {
			if (mkdir(path, 0700) == -1)
				goto error;
		}
		
		else
			goto error;
	}
	
	else if (!S_ISDIR(sb.st_mode)) {
		errno = ENOTDIR;
		goto error;
	}
	
	vc_snprintf(path, PATH_MAX, "%s/%s/%s", __PKGCONFDIR, name, dirname(file));
	
	if (lstat(path, &sb) == -1) {
		if (errno == ENOENT) {
			if (mkdir(path, 0700) == -1)
				goto error;
		}
		
		else
			goto error;
	}
	
	else if (!S_ISDIR(sb.st_mode)) {
		errno = ENOTDIR;
		goto error;
	}
	
	vc_snprintf(path, PATH_MAX, "%s/%s/%s", __PKGCONFDIR, name, file);
	
	if (lstat(path, &sb) == -1) {
		if (errno == ENOENT)
			goto open;
		
		else
			goto error;
	}
	
	else if (!S_ISDIR(sb.st_mode)) {
		errno = ENOTDIR;
		goto error;
	}
	
open:
	fd = open_trunc(path);
	
	if (fd == -1)
		goto error;
	
	if (write(fd, value, strlen(value)) == -1)
		goto error;
	
	if (write(fd, "\n", 1) == -1)
		goto error;
	
	free(file);
	return 0;
	
error:
	free(file);
	return -1;
}

/* bool methods */
int vc_cfg_get_bool(char *name, char *key, int *value)
{
	if (vc_cfg_isbool(key) == -1)
		return -1;
	
	char *buf;
	unsigned long len;
	
	len = _cfg_readkey(name, key, &buf);
	
	if (len == 0) {
		errno = ENOENT;
		return -1;
	}
	
	if (strncmp(buf, "true\n", len) == 0)
		*value = 1;
	
	else if (strncmp(buf, "false\n", len) == 0)
		*value = 0;
	
	else
		return -1;
	
	return 0;
}

int vc_cfg_set_bool(char *name, char *key, int value)
{
	if (vc_cfg_isbool(key) == -1)
		return -1;
	
	char *buf;
	
	if (value == 1)
		vc_asprintf(&buf, "true\n");
	
	else if (value == 0)
		vc_asprintf(&buf, "false\n");
	
	else {
		errno = EINVAL;
		return -1;
	}
	
	if (_cfg_writekey(name, key, buf) == -1)
		return -1;
	
	free(buf);
	return 0;
}

/* integer methods */
int vc_cfg_get_int(char *name, char *key, int *value)
{
	if (vc_cfg_isint(key) == -1)
		return -1;
	
	char *buf;
	unsigned long len;
	
	len = _cfg_readkey(name, key, &buf);
	
	if (len == 0) {
		errno = ENOENT;
		return -1;
	}
	
	*value = atoi(buf);
	
	return 0;
}

int vc_cfg_set_int(char *name, char *key, int value)
{
	if (vc_cfg_isint(key) == -1)
		return -1;
	
	char *buf;
	
	vc_asprintf(&buf, "%d\n", value);
	
	if (_cfg_writekey(name, key, buf) == -1)
		return -1;
	
	free(buf);
	return 0;
}

/* string methods */
int vc_cfg_get_str(char *name, char *key, char **value)
{
	if (vc_cfg_isstr(key) == -1)
		return -1;
	
	char *buf;
	unsigned long len;
	
	len = _cfg_readkey(name, key, &buf);
	
	if (len == 0) {
		errno = ENOENT;
		return -1;
	}
	
	*value = buf;
	
	return 0;
}

int vc_cfg_set_str(char *name, char *key, char *value)
{
	if (vc_cfg_isstr(key) == -1)
		return -1;
	
	char *buf;
	
	vc_asprintf(&buf, "%s\n", value);
	
	if (_cfg_writekey(name, key, buf) == -1)
		return -1;
	
	free(buf);
	return 0;
}

/* list methods */
int vc_cfg_get_list(char *name, char *key, char **value)
{
	if (vc_cfg_islist(key) == -1)
		return -1;
	
	char *buf;
	unsigned long len;
	
	len = _cfg_readkey(name, key, &buf);
	
	if (len == 0) {
		errno = ENOENT;
		return -1;
	}
	
	char *ptr;
	
	while ((ptr = strchr(buf, '\n')) != NULL)
		*ptr = ',';
	
	*value = buf;
	
	return 0;
}

int vc_cfg_set_list(char *name, char *key, char *value)
{
	if (vc_cfg_islist(key) == -1)
		return -1;
	
	char *ptr = NULL;
	
	while ((ptr = strchr(value, ',')) != NULL)
		*ptr = '\n';
	
	char *buf;
	
	vc_asprintf(&buf, "%s\n", value);
	
	if (_cfg_writekey(name, key, buf) == -1)
		return -1;
	
	free(buf);
	return 0;
}
