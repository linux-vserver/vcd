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
#include <wait.h>
#include <errno.h>
#include <arpa/inet.h>
#include <lucid/sys.h>

#include "vc.h"

int vc_nx_exists(nid_t nid)
{
	struct nx_info info;
	
	return nx_get_info(nid, &info) == -1 ? 0 : 1;
}

int vc_nx_new(nid_t nid)
{
	pid_t pid;
	int status;
	
	struct nx_create_flags flags = {
		.flags = NXF_PERSISTANT,
	};
	
	switch((pid = fork())) {
		case -1:
			return -1;
		
		case 0:
			if (nx_create(nid, &flags) == -1)
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

int vc_nx_release(nid_t nid)
{
	struct nx_flags flags = {
		.flags = 0,
		.mask  = NXF_PERSISTANT,
	};
	
	if (nx_set_flags(nid, &flags) == -1)
		return -1;
	
	return 0;
}

static inline
int _parse_cidr(char *cidr, uint32_t *ip, uint32_t *mask)
{
	struct in_addr ib;
	char *addr_ip, *addr_mask;
	
	*ip   = 0;
	*mask = 0;
	
	addr_ip   = strtok(cidr, "/");
	addr_mask = strtok(NULL, "/");
	
	if (addr_ip == 0)
		return -1;
	
	if (inet_aton(addr_ip, &ib) == -1)
		return -1;
	
	*ip = ib.s_addr;
	
	if (addr_mask == 0) {
		/* default to /24 */
		*mask = ntohl(0xffffff00);
	} else {
		if (strchr(addr_mask, '.') == 0) {
			/* We have CIDR notation */
			int sz = atoi(addr_mask);
			
			for (*mask = 0; sz > 0; --sz) {
				*mask >>= 1;
				*mask  |= 0x80000000;
			}
			
			*mask = ntohl(*mask);
		} else {
			/* Standard netmask notation */
			if (inet_aton(addr_mask, &ib) == -1)
				return -1;
			
			*mask = ib.s_addr;
		}
	}
	
	return 0;
}

int vc_nx_add_addr(nid_t nid, char *cidr)
{
	struct nx_addr addr;
	
	addr.type  = NXA_TYPE_IPV4;
	addr.count = 1;
	
	if (_parse_cidr(cidr, &addr.ip[0], &addr.mask[0]) == -1) {
		errno = EINVAL;
		return -1;
	}
	
	if (nx_add_addr(nid, &addr) == -1)
		return -1;
	
	return 0;
}

int vc_nx_rem_addr(nid_t nid, char *cidr)
{
	struct nx_addr addr;
	
	addr.type  = NXA_TYPE_ANY;
	
	if (nx_rem_addr(nid, &addr) == -1)
		return -1;
	
	return 0;
}
