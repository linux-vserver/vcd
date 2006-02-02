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

#include "list.h"
#include "printf.h"

uint32_t list32_getval(char *key, const list32_t list[])
{
	int i;
	
	for (i = 0; list[i].key; i++)
		if (strcasecmp(list[i].key, key) == 0)
			return list[i].val;
	
	return 0;
}

uint64_t list64_getval(char *key, const list64_t list[])
{
	int i;
	
	for (i = 0; list[i].key; i++)
		if (strcasecmp(list[i].key, key) == 0)
			return list[i].val;
	
	return 0;
}

char *list32_getkey(uint32_t val, const list32_t list[])
{
	int i;
	
	for (i = 0; list[i].key; i++)
		if (list[i].val == val)
			return list[i].key;
	
	return NULL;
}

char *list64_getkey(uint32_t val, const list64_t list[])
{
	int i;
	
	for (i = 0; list[i].key; i++)
		if (list[i].val == val)
			return list[i].key;
	
	return NULL;
}

void list32_parse(char *str, const list32_t list[],
                  uint32_t *flags, uint32_t *mask,
                  char clmod, char delim)
{
	char *p, *buf;
	int i, len;
	
	if (str == NULL)
		return;
	
	for (;;) {
		p = strchr(str, delim);
		
		if (p == NULL)
			len = strlen(str) + 1;
		else
			len = p - str + 1;
		
		buf = alloca(len);
		memset(buf, '\0', len);
		memcpy(buf, str, len - 1);
		
		if (*buf == clmod) {
			*flags &= ~list32_getval(++buf, list);
		} else {
			*flags |= list32_getval(buf, list);
		}
		
		*mask |= list32_getval(buf, list);
		
		free(buf);
		
		if (p == NULL)
			break;
		else
			str = ++p;
	}
	
	return;
}

void list64_parse(char *str, const list64_t list[],
                  uint64_t *flags, uint64_t *mask,
                  char clmod, char delim)
{
	char *p, *buf;
	int i, len;
	
	if (str == NULL)
		return;
	
	for (;;) {
		p = strchr(str, delim);
		
		if (p == NULL)
			len = strlen(str) + 1;
		else
			len = p - str + 1;
		
		buf = alloca(len);
		memset(buf, '\0', len);
		memcpy(buf, str, len - 1);
		
		if (*buf == clmod) {
			*flags &= ~list64_getval(++buf, list);
		} else {
			*flags |= list64_getval(buf, list);
		}
		
		*mask |= list64_getval(buf, list);
		
		free(buf);
		
		if (p == NULL)
			break;
		else
			str = ++p;
	}
	
	return;
}

char *list32_tostr(uint32_t val, const list32_t list[], char delim)
{
	int i, len = 0, idx = 0;
	
	for (i = 0; list[i].key; i++)
		if (list[i].val & val)
			len += vu_snprintf(NULL, 0, "%s%c", list[i].key, delim);
	
	char *str = malloc(len+1);
	memset(str, '\0', len+1);
	
	for (i = 0; list[i].key; i++)
		if (list[i].val & val)
			idx += vu_snprintf(str+idx, len-idx, "%s%c", list[i].key, delim);
	
	str[len-1] = '\0';
	
	return str;
}

char *list64_tostr(uint64_t val, const list64_t *list, char delim)
{
	int i, len = 0, idx = 0;
	
	for (i = 0; list[i].key; i++)
		if (list[i].val & val)
			len += vu_snprintf(NULL, 0, "%s%c", list[i].key, delim);
	
	char *str = malloc(len+1);
	memset(str, '\0', len+1);
	
	for (i = 0; list[i].key; i++)
		if (list[i].val & val)
			idx += vu_snprintf(str+idx, len-idx, "%s%c", list[i].key, delim);
	
	return str;
}
