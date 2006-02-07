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

#include "vc.h"

int vc_dx_add_path(char *name, char *path)
{
	xid_t xid;
	
	if (vc_cfg_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_dlimit_base base = {
		.filename = path,
		.flags    = 0,
	};
	
	if (vx_add_dlimit(xid, &base) == -1)
		return -1;
	
	return 0;
}

int vc_dx_rem_path(char *name, char *path)
{
	xid_t xid;
	
	if (vc_cfg_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_dlimit_base base = {
		.filename = path,
		.flags    = 0,
	};
	
	if (vx_rem_dlimit(xid, &base) == -1)
		return -1;
	
	return 0;
}

int vc_dx_get_limit(char *name, char *path,
                    uint32_t *spaceu, uint32_t *spacet,
                    uint32_t *inodesu, uint32_t *inodest,
                    uint32_t *reserved)
{
	xid_t xid;
	
	if (vc_cfg_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_dlimit dlimit = {
		.filename     = path,
		.space_used   = 0,
		.space_total  = 0,
		.inodes_used  = 0,
		.inodes_total = 0,
		.reserved     = 0,
	};
	
	if (vx_get_dlimit(xid, &dlimit) == -1)
		return -1;
	
	*spaceu   = dlimit.space_used;
	*spacet   = dlimit.space_total;
	*inodesu  = dlimit.inodes_used;
	*inodest  = dlimit.inodes_total;
	*reserved = dlimit.reserved;
	
	return 0;
}

int vc_dx_set_limit(char *name, char *path,
                    uint32_t spaceu, uint32_t spacet,
                    uint32_t inodesu, uint32_t inodest,
                    uint32_t reserved)
{
	xid_t xid;
	
	if (vc_cfg_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_dlimit dlimit = {
		.filename     = path,
		.space_used   = spaceu,
		.space_total  = spacet,
		.inodes_used  = inodesu,
		.inodes_total = inodest,
		.reserved     = reserved,
	};
	
	if (vx_set_dlimit(xid, &dlimit) == -1)
		return -1;
	
	return 0;
}
