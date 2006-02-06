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

#include <stdlib.h>
#include <wait.h>

#include "vconfig.h"
#include "vc.h"

#define CLONE_NEWNS 0x00020000

int vc_ns_new(char *name)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	pid_t pid;
	int status;
	signal(SIGCHLD, SIG_DFL);
	
	pid = sys_clone(CLONE_NEWNS|SIGCHLD, 0);
	
	switch(pid) {
		case -1:
			return -1;
		
		case 0:
			if (vx_set_namespace(xid) == -1)
				exit(EXIT_FAILURE);
			else
				exit(EXIT_SUCCESS);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				return -1;
		
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) == EXIT_SUCCESS)
					return 0;
				else
					return -1;
			}
			
			if (WIFSIGNALED(status)) {
				kill(getpid(), WTERMSIG(status));
				exit(EXIT_FAILURE);
			}
	}
	
	return 0;
}

int vc_ns_migrate(char *name)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	if (vx_enter_namespace(xid) == -1)
		return -1;
	
	return 0;
}
