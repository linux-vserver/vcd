/***************************************************************************
 *   Copyright 2005 by the vserver-utils team                              *
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

static const char single_dummy; /* prevent empty source file */

#ifdef LIBVCONFIG_BACKEND_SINGLE

#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <vserver.h>
#include <sys/mman.h>

#include <libowfat/mmap.h>
#include <libowfat/str.h>

#include "vc.h"
#include "pathconfig.h"
#include "vconfig.h"

/* backend specific map */
struct _single_node {
	char *key;
	char *path;
} _single_map[] = {
	{ "context.bcapabilities",   "bcapabilities" },
	{ "context.capabilities",    "ccapabilities" },
	{ "context.flags",           "flags" },
	{ "context.id",              "id" },
	{ "context.nice",            "nice", },
	{ "context.personality",     "personality" },
	{ "context.personalityflag", "personalityflag" },
	{ "context.sched",           "scheduler" },
	{ "init.runlevel",           "init/runlevel" },
	{ "init.style",              "init/style" },
	{ "limit.anon",              "limits/anon" },
	{ "limit.as",                "limits/as" },
	{ "limit.memlock",           "limits/memlock" },
	{ "limit.nofile",            "limits/nofile" },
	{ "limit.nproc",             "limits/nproc" },
	{ "limit.nsock",             "limits/nsock" },
	{ "limit.openfd",            "limits/openfd" },
	{ "limit.rss",               "limits/rss" },
	{ "limit.shmem",             "limits/shmem" },
	{ "namespace.fstab",         "namespace/fstab" },
	{ "namespace.mtab",          "namespace/mtab" },
	{ "namespace.root",          "namespace/root" },
	{ "net.addr",                "net/addr" },
	{ "uts.domainname",          "uts/domainname" },
	{ "uts.machine",             "uts/machine" },
	{ "uts.nodename",            "uts/nodename" },
	{ "uts.release",             "uts/release" },
	{ "uts.sysname",             "uts/sysname" },
	{ "uts.version",             "uts/version" },
	{ "vps.shell",               "shell" },
	{ "vps.timeout",             "timeout" },
};

#define NR_NODES (sizeof(_single_map)/sizeof(_single_map[0]))

/* errno buffer */
int errno_orig;

/* helper methods */
static
int _single_comp(const void *n1, const void *n2)
{
	struct _single_node *node1 = (struct _single_node *) n1;
	struct _single_node *node2 = (struct _single_node *) n2;
	return strcmp(node1->key, node2->key);
}

static
unsigned long _single_readkey(char *name, char *key, char **buf)
{
	struct _single_node n, *res;
	
	n.key = key;
	res = bsearch(&n, _single_map, NR_NODES, sizeof(struct _single_node), _single_comp);
	
	if (res == NULL)
		return 0;
	
	char path[PATH_MAX];
	vc_snprintf(path, PATH_MAX, "%s/%s/%s", __PKGCONFDIR, name, res->path);
	
	unsigned long len;
	*buf = mmap_private(path, &len);
	
	if (!buf)
		return 0;
	
	return len;
}

static
int _single_writekey(char *name, char *key, char *value)
{
	struct _single_node n, *res;
	
	n.key = key;
	res = bsearch(&n, _single_map, NR_NODES, sizeof(struct _single_node), _single_comp);
	
	if (res == NULL)
		return -1;
	
	char path[PATH_MAX];
	vc_snprintf(path, PATH_MAX, "%s/%s/%s", __PKGCONFDIR, name, res->path);
	
	char *buf;
	unsigned long len;
	
	buf = mmap_shared(path, &len);
	
	if (!buf)
		return -1;
	
	buf = mremap(buf, len, strlen(value), 0);
	
	if (buf == MAP_FAILED)
		return -1;
	
	buf = value;
	munmap(buf, len);
	
	return 0;
}

/* bool methods */
int vconfig_get_bool(char *name, char *key, int *value)
{
	if (vconfig_isbool(key) == -1)
		return -1;
	
	char *buf;
	unsigned long len;
	
	len = _single_readkey(name, key, &buf);
	
	if (len == 0)
		return -1;
	
	if (str_diffn(buf, "true\n", len))
		*value = 1;
	
	else if (str_diffn(buf, "false\n", len))
		*value = 0;
	
	else
		return -1;
	
	return 0;
}

int vconfig_set_bool(char *name, char *key, int value)
{
	if (vconfig_isbool(key) == -1)
		return -1;
	
	int rc;
	char *buf;
	
	if (value == 1)
		vc_asprintf(&buf, "true\n");
	else if (value == 0)
		vc_asprintf(&buf, "false\n");
	else
		return -1;
	
	rc = _single_writekey(name, key, buf);
	
	free(buf);
	return rc;
}

/* integer methods */
int vconfig_get_int(char *name, char *key, int *value)
{
	if (vconfig_isint(key) == -1)
		return -1;
	
	char *buf;
	unsigned long len;
	
	len = _single_readkey(name, key, &buf);
	
	if (len == 0)
		return -1;
	
	*value = atoi(buf);
	
	return 0;
}

int vconfig_set_int(char *name, char *key, int value)
{
	if (vconfig_isint(key) == -1)
		return -1;
	
	int rc;
	char *buf;
	
	vc_asprintf(&buf, "%d\n", value);
	
	rc = _single_writekey(name, key, buf);
	
	free(buf);
	return rc;
}

/* string methods */
int vconfig_get_str(char *name, char *key, char **value)
{
	if (vconfig_isstr(key) == -1)
		return -1;
	
	char *buf;
	unsigned long len;
	
	len = _single_readkey(name, key, &buf);
	
	if (len == 0)
		return -1;
	
	*value = buf;
	
	return 0;
}

int vconfig_set_str(char *name, char *key, char *value)
{
	if (vconfig_isstr(key) == -1)
		return -1;
	
	int rc;
	char *buf;
	
	vc_asprintf(&buf, "%s\n", value);
	
	rc = _single_writekey(name, key, buf);
	
	free(buf);
	return rc;
}

/* list methods */
int vconfig_get_list(char *name, char *key, char **value)
{
	if (vconfig_islist(key) == -1)
		return -1;
	
	char *buf;
	unsigned long len;
	
	len = _single_readkey(name, key, &buf);
	
	if (len == 0)
		return -1;
	
	char *ptr = buf + len - 1;
	
	if (*ptr == '\n')
		*ptr = '\0';
	
	while ((ptr = strchr(buf, '\n')) != NULL)
		*ptr = ',';
	
	*value = buf;
	
	return 0;
}

int vconfig_set_list(char *name, char *key, char *value)
{
	if (vconfig_islist(key) == -1)
		return -1;
	
	int rc;
	char *ptr = NULL;
	
	while ((ptr = strchr(value, ',')) != NULL)
		*ptr = '\n';
	
	char *buf;
	
	vc_asprintf(&buf, "%s\n", value);
	
	rc = _single_writekey(name, key, buf);
	
	free(buf);
	return rc;
}

#endif
