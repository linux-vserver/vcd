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

#ifndef _LIBVCONFIG_MAP_H
#define _LIBVCONFIG_MAP_H

#include <string.h>
#include "node.h"

static struct vconfig_node vconfig_map[] = {
	{ "context.bcapabilities",   VCONFIG_LIST_T },
	{ "context.capabilities",    VCONFIG_LIST_T },
	{ "context.flags",           VCONFIG_LIST_T },
	{ "context.id",              VCONFIG_INT_T },
	{ "context.nice",            VCONFIG_INT_T },
	{ "context.personality",     VCONFIG_LIST_T },
	{ "context.personalityflag", VCONFIG_LIST_T },
	{ "context.sched",           VCONFIG_LIST_T },
	{ "limit.anon",              VCONFIG_LIST_T },
	{ "limit.as",                VCONFIG_LIST_T },
	{ "limit.memlock",           VCONFIG_LIST_T },
	{ "limit.nofile",            VCONFIG_LIST_T },
	{ "limit.nproc",             VCONFIG_LIST_T },
	{ "limit.nsock",             VCONFIG_LIST_T },
	{ "limit.openfd",            VCONFIG_LIST_T },
	{ "limit.rss",               VCONFIG_LIST_T },
	{ "limit.shmem",             VCONFIG_LIST_T },
	{ "namespace.fstab",         VCONFIG_STR_T },
	{ "namespace.mtab",          VCONFIG_STR_T },
	{ "namespace.root",          VCONFIG_STR_T },
	{ "net.addr",                VCONFIG_STR_T },
	{ "uts.domainname",          VCONFIG_STR_T },
	{ "uts.machine",             VCONFIG_STR_T },
	{ "uts.nodename",            VCONFIG_STR_T },
	{ "uts.release",             VCONFIG_STR_T },
	{ "uts.sysname",             VCONFIG_STR_T },
	{ "uts.version",             VCONFIG_STR_T },
	{ "vps.init",                VCONFIG_STR_T },
	{ "vps.shell",               VCONFIG_STR_T },
	{ "vps.timeout",             VCONFIG_INT_T },
};

#define NR_MAP_ENTRIES (sizeof(vconfig_map)/sizeof(vconfig_map[0]))

static inline
int _vconfig_comp(const void *n1, const void *n2)
{
	struct vconfig_node *node1 = (struct vconfig_node *) n1;
	struct vconfig_node *node2 = (struct vconfig_node *) n2;
	return strcmp(node1->key, node2->key);
}

#endif
