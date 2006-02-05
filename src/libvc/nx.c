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
#include <arpa/inet.h>

#include "printf.h"
#include "vc.h"

int vc_nx_exists(nid_t nid)
{
	struct nx_info info;
	
	return nx_get_info(nid, &info) == -1 ? 0 : 1;
}

int vc_nx_create(nid_t nid, char *flagstr)
{
	struct nx_create_flags flags = {
		.flags = 0,
	};
	
	if (flagstr == NULL)
		goto create;
	
	uint64_t mask = 0;
	
	if (list64_parse(flagstr, nflags_list, &flags.flags, &mask, '~', ',') == -1)
		return -1;
	
create:
	if (nx_create(nid, &flags) == -1)
		return -1;
	
	return 0;
}

int vc_nx_new(nid_t nid, char *flagstr)
{
	pid_t pid;
	int status;
	char *buf;
	
	switch((pid = fork())) {
		case -1:
			return -1;
		
		case 0:
			vu_asprintf(&buf, "%s,%s", flagstr, "PERSISTANT");
			
			if (vc_nx_create(nid, buf) == -1)
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

int vc_nx_migrate(nid_t nid)
{
	if (!vc_nx_exists(nid)) {
		if (vc_nx_create(nid, NULL) == -1)
			return -1;
	}
	
	else {
		if (nx_migrate(nid) == -1)
			return -1;
	}
	
	return 0;
}

int vc_nx_get_flags(nid_t nid, char **flagstr, uint64_t *flags)
{
	struct nx_flags nflags = {
		.flags = 0,
		.mask  = 0,
	};
	
	if (nx_get_flags(nid, &nflags) == -1)
		return -1;
	
	if (flags != NULL)
		*flags = nflags.flags;
	
	if (list64_tostr(nflags_list, nflags.flags, flagstr, ',') == -1)
		return -1;
	
	return 0;
}

int vc_nx_set_flags(nid_t nid, char *flagstr)
{
	struct nx_flags flags = {
		.flags = 0,
		.mask  = 0,
	};
	
	if (list64_parse(flagstr, nflags_list, &flags.flags, &flags.mask, '~', ',') == -1)
		return -1;
	
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
	
	if (_parse_cidr(cidr, &addr.ip[0], &addr.mask[0]) == -1)
		return -1;
	
	if (nx_add_addr(nid, &addr) == -1)
		return -1;
	
	return 0;
}

int vc_nx_rem_addr(nid_t nid, char *cidr)
{
	struct nx_addr addr;
	
	addr.type  = NXA_TYPE_IPV4;
	addr.count = 1;
	
	if (_parse_cidr(cidr, &addr.ip[0], &addr.mask[0]) == -1)
		return -1;
	
	if (nx_rem_addr(nid, &addr) == -1)
		return -1;
	
	return 0;
}
