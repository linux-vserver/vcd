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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <linux/vserver/cvirt_cmd.h>

#include "node.h"
#include "map.h"
#include "vconfig.h"

int vconfig_isbool(char *key)
{
	struct vconfig_node buf, *res;
	
	buf.key = key;
	res = bsearch(&buf, vconfig_map, NR_MAP_ENTRIES, sizeof(struct vconfig_node), _vconfig_comp);
	
	if (res == NULL)
		return -1;
	
	if (res->type != VCONFIG_BOOL_T)
		return -1;
	
	return 0;
}

int vconfig_isint(char *key)
{
	struct vconfig_node buf, *res;
	
	buf.key = key;
	res = bsearch(&buf, vconfig_map, NR_MAP_ENTRIES, sizeof(struct vconfig_node), _vconfig_comp);
	
	if (res == NULL)
		return -1;
	
	if (res->type != VCONFIG_INT_T)
		return -1;
	
	return 0;
}

int vconfig_isstr(char key[])
{
	struct vconfig_node buf, *res;
	
	buf.key = key;
	res = bsearch(&buf, vconfig_map, NR_MAP_ENTRIES, sizeof(struct vconfig_node), _vconfig_comp);
	
	if (res == NULL)
		return -1;
	
	if (res->type != VCONFIG_STR_T)
		return -1;
	
	return 0;
}

int vconfig_islist(char key[])
{
	struct vconfig_node buf, *res;
	
	buf.key = key;
	res = bsearch(&buf, vconfig_map, NR_MAP_ENTRIES, sizeof(struct vconfig_node), _vconfig_comp);
	
	if (res == NULL)
		return -1;
	
	if (res->type != VCONFIG_LIST_T)
		return -1;
	
	return 0;
}

void vconfig_print_nodes(void)
{
	unsigned int i;
	
	for (i = 0; i < NR_MAP_ENTRIES; i++) {
		printf("%s\n", vconfig_map[i].key);
	}
}

#ifdef LIBVCONFIG_BACKEND_SINGLE
#include "single.c"
#endif

#ifndef LIBVCONFIG_HAVE_BACKEND
#include "dummy.c"
#endif

xid_t vconfig_get_xid(char *name)
{
	int buf = vconfig_get_int(name, "context.id");
	struct vx_vhi_name vhi_name;
	vhi_name.field = VHIN_CONTEXT;
	
	if (vx_get_vhi_name((xid_t) buf, &vhi_name) == -1)
		return -1;
	
	if (strcmp(vhi_name.name, name) != 0)
		/* TODO: probably implement search */
		return -1;
	
	return (xid_t) buf;
}

char *vconfig_get_name(xid_t xid)
{
	struct vx_vhi_name vhi_name;
	vhi_name.field = VHIN_CONTEXT;
	char *buf = malloc(65);
	
	if (vx_get_vhi_name(xid, &vhi_name) == -1)
		return NULL;
	
	memcpy(buf, vhi_name.name, 65);
	
	return buf;
}
