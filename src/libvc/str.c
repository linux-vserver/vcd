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
#include <ctype.h>
#include <arpa/inet.h>

#include "vc.h"

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

uint64_t vc_str_to_dlim(char *str)
{
	if (str == NULL)
		return CDLIM_KEEP;
	
	if (strcmp(str, "inf") == 0)
		return CDLIM_INFINITY;
	
	if (strcmp(str, "keep") == 0)
		return CDLIM_KEEP;
	
	return atoi(str);
}

char *vc_dlim_to_str(uint64_t lim)
{
	char *buf;
	
	if (lim == CDLIM_INFINITY)
		vc_asprintf(&buf, "%s", "inf");
	
	vc_asprintf(&buf, "%d", lim);
	
	return buf;
}

int vc_str_to_addr(char *str, uint32_t *ip, uint32_t *mask)
{
	struct in_addr ib;
	char *addr_ip, *addr_mask;
	
	*ip   = 0;
	*mask = 0;
	
	addr_ip   = strtok(str, "/");
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
