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
#include <vserver.h>

#include <lucid/mmap.h>
#include <lucid/open.h>

#include "pathconfig.h"
#include "vc.h"

/* errno buffer */
int errno_orig;

/* map of valid nodes */
vc_cfg_node_t vc_cfg_map[] = {
	{ "init.runlevel",     VC_CFG_STR_T  },
	{ "init.style",        VC_CFG_STR_T  },
	{ "limit.anon",        VC_CFG_LIST_T },
	{ "limit.as",          VC_CFG_LIST_T },
	{ "limit.memlock",     VC_CFG_LIST_T },
	{ "limit.nofile",      VC_CFG_LIST_T },
	{ "limit.nproc",       VC_CFG_LIST_T },
	{ "limit.rss",         VC_CFG_LIST_T },
	{ "ns.fstab",          VC_CFG_STR_T  },
	{ "ns.mtab",           VC_CFG_STR_T  },
	{ "ns.root",           VC_CFG_STR_T  },
	{ "nx.addr",           VC_CFG_LIST_T },
	{ "nx.flags",          VC_CFG_LIST_T },
	{ "vhi.domainname",    VC_CFG_STR_T  },
	{ "vhi.machine",       VC_CFG_STR_T  },
	{ "vhi.nodename",      VC_CFG_STR_T  },
	{ "vhi.release",       VC_CFG_STR_T  },
	{ "vhi.sysname",       VC_CFG_STR_T  },
	{ "vhi.version",       VC_CFG_STR_T  },
	{ "vps.shell",         VC_CFG_STR_T  },
	{ "vps.timeout",       VC_CFG_INT_T  },
	{ "vx.bcaps",          VC_CFG_LIST_T },
	{ "vx.ccaps",          VC_CFG_LIST_T },
	{ "vx.flags",          VC_CFG_LIST_T },
	{ "vx.id",             VC_CFG_INT_T  },
	{ "vx.sched",          VC_CFG_LIST_T },
	{ NULL,                0             },
};

static
size_t _cfg_readkey(char *name, char *key, char **buf)
{
	char path[PATH_MAX];
	vc_snprintf(path, PATH_MAX, "%s/%s/%s", __PKGCONFDIR, name, key);
	
	size_t len;
	char *buf2;
	buf2 = mmap_private(path, &len);
	
	if (buf2 == NULL || len == 0)
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
	char path[PATH_MAX];
	struct stat sb;
	
	/* check PKGCONFDIR but do not create it */
	if (lstat(__PKGCONFDIR, &sb) == -1)
		return -1;
	
	if (!S_ISDIR(sb.st_mode)) {
		errno = ENOTDIR;
		return -1;
	}
	
	/* check confdir for <name> and create if necessary */
	vc_snprintf(path, PATH_MAX, "%s/%s", __PKGCONFDIR, name);
	
	if (lstat(path, &sb) == -1) {
		if (errno == ENOENT) {
			if (mkdir(path, 0700) == -1)
				return -1;
		}
		
		else
			return -1;
	}
	
	else if (!S_ISDIR(sb.st_mode)) {
		errno = ENOTDIR;
		return -1;
	}
	
	/* write <value> to <key> */
	vc_snprintf(path, PATH_MAX, "%s/%s/%s", __PKGCONFDIR, name, key);
	
	fd = open_trunc(path);
	
	if (fd == -1)
		return -1;;
	
	if (write(fd, value, strlen(value)) == -1)
		return -1;
	
	if (write(fd, "\n", 1) == -1)
		return -1;
	
	return 0;
}

int vc_cfg_get_type(char *key)
{
	int i;
	
	for (i = 0; vc_cfg_map[i].key; i++)
		if (strcasecmp(vc_cfg_map[i].key, key) == 0)
			return vc_cfg_map[i].type;
	
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

/* bool methods */
int vc_cfg_get_bool(char *name, char *key, int *value)
{
	if (vc_cfg_isbool(key) == -1)
		return -1;
	
	char *buf;
	size_t len;
	
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
	
	if (value == 1) {
		if (_cfg_writekey(name, key, "true") == -1)
			return -1;
	}
	
	else if (value == 0) {
		if (_cfg_writekey(name, key, "false") == -1)
			return -1;
	}
	
	else {
		errno = EINVAL;
		return -1;
	}
	
	return 0;
}

/* integer methods */
int vc_cfg_get_int(char *name, char *key, int *value)
{
	if (vc_cfg_isint(key) == -1)
		return -1;
	
	char *buf;
	size_t len;
	
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
	
	int rc = 0;
	char *buf;
	
	vc_asprintf(&buf, "%d\n", value);
	
	if (_cfg_writekey(name, key, buf) == -1)
		rc = -1;
	
	free(buf);
	return rc;
}

/* string methods */
int vc_cfg_get_str(char *name, char *key, char **value)
{
	if (vc_cfg_isstr(key) == -1)
		return -1;
	
	char *buf;
	size_t len;
	
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
	
	return _cfg_writekey(name, key, value);
}

/* list methods */
int vc_cfg_get_list(char *name, char *key, char **value)
{
	if (vc_cfg_islist(key) == -1)
		return -1;
	
	char *buf;
	size_t len;
	
	len = _cfg_readkey(name, key, &buf);
	
	if (len == 0) {
		errno = ENOENT;
		return -1;
	}
	
	*value = buf;
	
	return 0;
}

int vc_cfg_set_list(char *name, char *key, char *value)
{
	if (vc_cfg_islist(key) == -1)
		return -1;
	
	return _cfg_writekey(name, key, value);
}
