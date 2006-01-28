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

#ifndef LIBVCONFIG_HAVE_BACKEND
#define LIBVCONFIG_HAVE_BACKEND

#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <vserver.h>

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
	{ "vps.init",                "init" },
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
int _single_open(char *name, char *key, int flags)
{
	struct _single_node buf, *res;
	
	buf.key = key;
	res = bsearch(&buf, _single_map, NR_NODES, sizeof(struct _single_node), _single_comp);
	
	if (res == NULL)
		return -1;
	
	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s/%s/%s", __PKGCONFDIR, name, res->path);
	
	return open(path, O_RDWR|flags, 0600);
}

/* bool methods */
int vconfig_get_bool(char *name, char *key)
{
	if (vconfig_isbool(key) == -1)
		return -1;
	
	int fd = _single_open(name, key, 0);
	
	if (fd == -1)
		goto error;
	
	int i;
	char buf[6];
	memset(buf, '\0', 6);
	
	/* we support true/false (i.e. up to 5 chars max) */
	for(i = 0; i <= 5; i++) {
		if (read(fd, &buf[i], 1) == -1)
			goto error;
		 
		if (buf[i] == '\0' || buf[i] == '\n') {
			buf[i] = '\0';
			break;
		}
	}
	
	close(fd);
	
	if (strcmp(buf, "true") == 0)
		return 1;
	
	return 0;
	
error:
	errno_orig = errno;
	close(fd);
	errno = errno_orig;
	return -1;
}

int vconfig_set_bool(char *name, char *key, int value)
{
	if (vconfig_isbool(key) == -1)
		return -1;
	
	int fd = _single_open(name, key, O_CREAT|O_TRUNC);
	
	if (fd == -1)
		goto error;
	
	char buf[6];
	size_t len;
	
	/* we support up to 32 bit int (i.e. 10 digits max) */
	if (value == 1)
		len = snprintf(buf, 6, "true");
	else
		len = snprintf(buf, 6, "false");
	
	if (len > 6 || len == 0)
		goto error;
	
	if (write(fd, buf, 6) == -1)
		goto error;
	
	if (write(fd, "\n", 1) == -1)
		goto error;
	
	close(fd);
	return 0;
	
error:
	errno_orig = errno;
	close(fd);
	errno = errno_orig;
	return -1;
}

/* integer methods */
int vconfig_get_int(char *name, char *key)
{
	if (vconfig_isint(key) == -1)
		return -1;
	
	int fd = _single_open(name, key, 0);
	
	if (fd == -1)
		goto error;
	
	int i;
	char buf[11];
	memset(buf, '\0', 11);
	
	/* we support up to 32 bit int (i.e. 10 digits max) */
	for(i = 0; i <= 10; i++) {
		if (read(fd, &buf[i], 1) == -1)
			goto error;
		 
		if (buf[i] == '\0' || buf[i] == '\n') {
			buf[i] = '\0';
			break;
		}
	}
	
	close(fd);
	return atoi(buf);
	
error:
	errno_orig = errno;
	close(fd);
	errno = errno_orig;
	return -1;
}

int vconfig_set_int(char *name, char *key, int value)
{
	if (vconfig_isint(key) == -1)
		return -1;
	
	int fd = _single_open(name, key, O_CREAT|O_TRUNC);
	
	if (fd == -1)
		goto error;
	
	char buf[11];
	size_t len;
	
	/* we support up to 32 bit int (i.e. 10 digits max) */
	len = snprintf(buf, 11, "%d", value);
	
	if (len > 11 || len == 0)
		goto error;
	
	if (write(fd, buf, len) == -1)
		goto error;
	
	if (write(fd, "\n", 1) == -1)
		goto error;
	
	close(fd);
	return 0;
	
error:
	errno_orig = errno;
	close(fd);
	errno = errno_orig;
	return -1;
}

/* string methods */
char *vconfig_get_str(char *name, char *key)
{
	if (vconfig_isstr(key) == -1)
		return NULL;
	
	int fd = _single_open(name, key, 0);
	
	if (fd == -1)
		goto error;
	
	off_t len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	
	int i;
	char *buf = malloc(len+1);
	memset(buf, '\0', len+1);
	
	/* read whole file */
	for(i = 0; i <= len; i++) {
		if (read(fd, buf+i, 1) == -1)
			goto error;
	}
	
	char *ptr = buf+len-1;
	if (*ptr == '\n')
		*ptr = '\0';
	
	close(fd);
	return buf;
	
error:
	errno_orig = errno;
	close(fd);
	errno = errno_orig;
	return NULL;
}

int vconfig_set_str(char *name, char *key, char *value)
{
	if (vconfig_isstr(key) == -1)
		return -1;
	
	int fd = _single_open(name, key, O_CREAT|O_TRUNC);
	
	if (fd == -1)
		goto error;
	
	if (write(fd, value, strlen(value)) == -1)
		goto error;
	
	if (write(fd, "\n", 1) == -1)
		goto error;
	
	close(fd);
	return 0;
	
error:
	errno_orig = errno;
	close(fd);
	errno = errno_orig;
	return -1;
}

/* list methods */
char *vconfig_get_list(char *name, char *key)
{
	if (vconfig_islist(key) == -1)
		return NULL;
	
	int fd = _single_open(name, key, 0);
	
	if (fd == -1)
		goto error;
	
	off_t len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	
	int i;
	char *buf = malloc(len+1);
	memset(buf, '\0', len+1);
	
	/* read whole file */
	for(i = 0; i <= len; i++) {
		if (read(fd, buf+i, 1) == -1)
			goto error;
	}
	
	char *ptr = buf+len-1;
	if (*ptr == '\n')
		*ptr = '\0';
	
	close(fd);
	
	while ((ptr = strchr(buf, '\n')) != NULL)
		*ptr = ',';
	
	return buf;
	
error:
	errno_orig = errno;
	close(fd);
	errno = errno_orig;
	return NULL;
}

int vconfig_set_list(char *name, char *key, char *value)
{
	if (vconfig_islist(key) == -1)
		return -1;
	
	int fd = _single_open(name, key, O_CREAT|O_TRUNC);
	
	if (fd == -1)
		goto error;
	
	char *ptr;
	
	while ((ptr = strchr(value, ',')) != NULL)
		*ptr = '\n';
	
	if (write(fd, value, strlen(value)) == -1)
		goto error;
	
	if (write(fd, "\n", 1) == -1)
		goto error;
	
	close(fd);
	return 0;
	
error:
	errno_orig = errno;
	close(fd);
	errno = errno_orig;
	return -1;
}

#endif
