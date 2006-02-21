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

#include <stdlib.h>
#include <errno.h>
#include <wait.h>
#include <lucid/sys.h>

#include "vc.h"

int vc_vx_exists(xid_t xid)
{
	struct vx_info info;
	
	return vx_get_info(xid, &info) == -1 ? 0 : 1;
}

int vc_vx_new(xid_t xid)
{
	pid_t pid;
	int status;
	
	struct vx_create_flags flags = {
		.flags = VXF_PERSISTANT,
	};
	
	switch((pid = fork())) {
		case -1:
			return -1;
		
		case 0:
			if (vx_create(xid, &flags) == -1)
				exit(errno);
			
			exit(EXIT_SUCCESS);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				return -1;
			
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) == EXIT_SUCCESS)
					return 0;
				else {
					errno = WEXITSTATUS(status);
					return -1;
				}
			}
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
	}
	
	return 0;
}

int vc_vx_release(xid_t xid)
{
	struct vx_flags flags = {
		.flags = 0,
		.mask  = VXF_PERSISTANT,
	};
	
	if (vx_set_flags(xid, &flags) == -1)
		return -1;
	
	return 0;
}

uint64_t vc_str_to_rlim(char *str)
{
	if (str == NULL)
		return CRLIM_KEEP;
	
	if (strcmp(str, "inf") == 0)
		return CRLIM_INFINITY;
	
	if (strcmp(str, "keep") == 0)
		return CRLIM_KEEP;
	
	return atoi(str);
}

char *vc_rlim_to_str(uint64_t lim)
{
	char *buf;
	
	if (lim == CRLIM_INFINITY)
		vc_asprintf(&buf, "%s", "inf");
	
	vc_asprintf(&buf, "%d", lim);
	
	return buf;
}
