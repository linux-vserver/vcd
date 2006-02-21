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
#include <ctype.h>
#include <lucid/sys.h>

#include "vc.h"

#ifndef CLONE_NEWNS
#define CLONE_NEWNS 0x00020000
#endif

int vc_ns_new(xid_t xid)
{
	pid_t pid;
	int status;
	
	switch((pid = sys_clone(CLONE_NEWNS|SIGCHLD, 0))) {
		case -1:
			return -1;
		
		case 0:
			if (vx_set_namespace(xid) == -1)
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

int vc_str_to_fstab(char *str, char **src, char **dst, char **type, char **data)
{
	*src = str;
	while (!isspace(*str) && *str != '\0') ++str;
	if (*str == '\0') goto error;
	*str++ = '\0';
	while (isspace(*str)) ++str;
	
	*dst = str;
	while (!isspace(*str) && *str != '\0') ++str;
	if (*str == '\0') goto error;
	*str++ = '\0';
	while (isspace(*str)) ++str;
	
	*type = str;
	while (!isspace(*str) && *str != '\0') ++str;
	if (*str == '\0') goto error;
	*str++ = '\0';
	while (isspace(*str)) ++str;
	
	*data = str;
	while (!isspace(*str) && *str != '\0') ++str;
	*str++ = '\0';
	while (isspace(*str)) ++str;
	
	return 0;
	
error:
	errno = EINVAL;
	return -1;
}
