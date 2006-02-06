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
#include <vserver.h>

#include "vc.h"
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

int vconfig_print_nodes(void)
{
	int len = 0;
	unsigned int i;
	
	for (i = 0; i < NR_MAP_ENTRIES; i++) {
		len += vc_printf("%s\n", vconfig_map[i].key);
	}
	
	return len;
}

int vconfig_get_xid(char *name, xid_t *xid)
{
	int buf;
	
	if (vconfig_get_int(name, "context.id", &buf) == -1)
		return -1;
	
	*xid = (xid_t) buf;
	
	return 0;
}

int vconfig_get_name(xid_t xid, char **name)
{
	struct vx_vhi_name vhi_name;
	vhi_name.field = VHIN_CONTEXT;
	char *buf = malloc(65);
	
	if (vx_get_vhi_name(xid, &vhi_name) == -1)
		return -1;
	
	vc_asprintf(&buf, "%s", vhi_name.name);
	
	return 0;
}
