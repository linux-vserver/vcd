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

#include "vconfig.h"
#include "vc.h"

int vc_task_name(pid_t pid, char **name)
{
	xid_t xid = vx_get_task_xid(pid);
	
	if (vconfig_get_name(xid, name) == -1)
		return -1;
	
	return 0;
}

int vc_task_nid(pid_t pid, nid_t *nid)
{
	*nid = nx_get_task_nid(pid);
	
	return 0;
}

int vc_task_xid(pid_t pid, xid_t *xid)
{
	*xid = vx_get_task_xid(pid);
	
	return 0;
}
