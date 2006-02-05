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

int vc_in_get_xid(char *file, xid_t *xid)
{
	struct vx_iattr iattr = {
		.filename = file,
		.xid      = 0,
		.flags    = 0,
		.mask     = 0,
	};
	
	if (vx_get_iattr(&iattr) == -1)
		return -1;
	
	*xid = iattr.xid;
	
	return 0;
}

int vc_in_get_attr(char *file, char **flagstr, uint32_t *flags)
{
	struct vx_iattr iattr = {
		.filename = file,
		.xid      = 0,
		.flags    = 0,
		.mask     = 0,
	};
	
	if (vx_get_iattr(&iattr) == -1)
		return -1;
	
	if (flags != NULL)
		*flags = iattr.flags;
	
	if (list32_tostr(iattr_list, iattr.flags, flagstr, ',') == -1)
		return -1;
	
	return 0;
}

int vc_in_set_xid(char *file, xid_t xid)
{
	struct vx_iattr iattr = {
		.filename = file,
		.xid      = xid,
		.flags    = IATTR_XID,
		.mask     = IATTR_XID,
	};
	
	if (vx_set_iattr(&iattr) == -1)
		return -1;
	
	return 0;
}

int vc_in_set_attr(char *file, char *flagstr)
{
	struct vx_iattr iattr = {
		.filename = file,
		.xid      = 0,
		.flags    = 0,
		.mask     = 0,
	};
	
	if (list32_parse(flagstr, iattr_list, &iattr.flags, &iattr.mask, '~', ',') == -1)
		return -1;
	
	if (vx_set_iattr(&iattr) == -1)
		return -1;
	
	return 0;
}
