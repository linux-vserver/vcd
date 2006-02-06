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

#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <alloca.h>
#include <errno.h>

#include "vc.h"

/* list search */
int vc_list32_getval(const vc_list32_t list[], char *key, uint32_t *val)
{
	int i;
	
	for (i = 0; list[i].key; i++) {
		if (strcasecmp(list[i].key, key) == 0) {
			*val = list[i].val;
			return 0;
		}
	}
	
	errno = EINVAL;
	return -1;
}

int vc_list64_getval(const vc_list64_t list[], char *key, uint64_t *val)
{
	int i;
	
	for (i = 0; list[i].key; i++) {
		if (strcasecmp(list[i].key, key) == 0) {
			*val = list[i].val;
			return 0;
		}
	}
	
	errno = EINVAL;
	return -1;
}

int vc_list32_getkey(const vc_list32_t list[], uint32_t val, char **key)
{
	int i;
	
	for (i = 0; list[i].key; i++) {
		if (list[i].val == val) {
			vc_asprintf(key, "%s", list[i].key);
			return 0;
		}
	}
	
	errno = EINVAL;
	return -1;
}

int vc_list64_getkey(const vc_list64_t list[], uint64_t val, char **key)
{
	int i;
	
	for (i = 0; list[i].key; i++) {
		if (list[i].val == val) {
			vc_asprintf(key, "%s", list[i].key);
			return 0;
		}
	}
	
	errno = EINVAL;
	return -1;
}

/* list parser */
int vc_list32_parse(char *str, const vc_list32_t list[],
                 uint32_t *flags, uint32_t *mask,
                 char clmod, char delim)
{
	char *p, *buf;
	int i, len, clear = 0;
	uint32_t cur_flag;
	
	if (str == NULL) {
		errno = EINVAL;
		return -1;
	}
	
	for (;;) {
		p = strchr(str, delim);
		
		if (p == NULL)
			len = strlen(str) + 1;
		else
			len = p - str + 1;
		
		buf = malloc(len);
		
		if (buf == NULL)
			return -1;
		
		memset(buf, '\0', len);
		memcpy(buf, str, len - 1);
		
		if (*buf == clmod) {
			clear = 1;
			++buf;
		}
		
		if (vc_list32_getval(list, buf, &cur_flag) == -1)
			return -1;
		
		if (clear) {
			*flags &= ~cur_flag;
			*mask  |=  cur_flag;
		} else {
			*flags |=  cur_flag;
			*mask  |=  cur_flag;
		}
		
		free(buf);
		
		if (p == NULL)
			break;
		else
			str = ++p;
	}
	
	return 0;
}

int vc_list64_parse(char *str, const vc_list64_t list[],
                 uint64_t *flags, uint64_t *mask,
                 char clmod, char delim)
{
	char *p, *buf;
	int i, len, clear = 0;
	uint64_t cur_flag;
	
	if (str == NULL) {
		errno = EINVAL;
		return -1;
	}
	
	for (;;) {
		p = strchr(str, delim);
		
		if (p == NULL)
			len = strlen(str) + 1;
		else
			len = p - str + 1;
		
		buf = malloc(len);
		
		if (buf == NULL)
			return -1;
		
		memset(buf, '\0', len);
		memcpy(buf, str, len - 1);
		
		if (*buf == clmod) {
			clear = 1;
			++buf;
		}
		
		if (vc_list64_getval(list, buf, &cur_flag) == -1)
			return -1;
		
		if (clear) {
			*flags &= ~cur_flag;
			*mask  |=  cur_flag;
		} else {
			*flags |=  cur_flag;
			*mask  |=  cur_flag;
		}
		
		free(buf);
		
		if (p == NULL)
			break;
		else
			str = ++p;
	}
	
	return 0;
}

/* list converter */
int vc_list32_tostr(const vc_list32_t list[], uint32_t val, char **str, char delim)
{
	int i, len = 0, idx = 0;
	char *p = *str;
	
	for (i = 0; list[i].key; i++)
		if (list[i].val & val)
			len += vc_snprintf(NULL, 0, "%s%c", list[i].key, delim);
	
	p = malloc(len+1);
	
	if (p == NULL)
		return -1;
	
	memset(p, '\0', len+1);
	
	for (i = 0; list[i].key; i++)
		if (list[i].val & val)
			idx += vc_snprintf(p+idx, len-idx, "%s%c", list[i].key, delim);
	
	return 0;
}

int vc_list64_tostr(const vc_list64_t list[], uint64_t val, char **str, char delim)
{
	int i, len = 0, idx = 0;
	char *p = *str;
	
	for (i = 0; list[i].key; i++)
		if (list[i].val & val)
			len += vc_snprintf(NULL, 0, "%s%c", list[i].key, delim);
	
	p = malloc(len+1);
	
	if (p == NULL)
		return -1;
	
	memset(p, '\0', len+1);
	
	for (i = 0; list[i].key; i++)
		if (list[i].val & val)
			idx += vc_snprintf(p+idx, len-idx, "%s%c", list[i].key, delim);
	
	return 0;
}
