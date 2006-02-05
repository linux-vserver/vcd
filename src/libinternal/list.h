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

#ifndef _INTERNAL_LIST_H
#define _INTERNAL_LIST_H

#include <stdint.h>

/* list types */
typedef struct {
	char *key;
	uint32_t val;
} list32_t;

typedef struct {
	char *key;
	uint64_t val;
} list64_t;

/* list macros */
#define LIST32_START(LIST) const list32_t LIST[] = {
#define LIST32_NODE(PREFIX, NAME) { #NAME, PREFIX ## _ ## NAME },
#define LIST32_END { NULL, 0 } };

#define LIST64_START(LIST) const list64_t LIST[] = {
#define LIST64_NODE(PREFIX, NAME) { #NAME, PREFIX ## _ ## NAME },
#define LIST64_END { NULL, 0 } };

/* list methods */
int list32_getval(const list32_t list[], char *key, uint32_t *val);
int list64_getval(const list64_t list[], char *key, uint64_t *val);
int list32_getkey(const list32_t list[], uint32_t val, char **key);
int list64_getkey(const list64_t list[], uint64_t val, char **key);
int list32_parse(char *str, const list32_t list[],
                 uint32_t *flags, uint32_t *mask,
                 char clmod, char delim);
int list64_parse(char *str, const list64_t list[],
                 uint64_t *flags, uint64_t *mask,
                 char clmod, char delim);
int list32_tostr(const list32_t list[], uint32_t val, char **str, char delim);
int list64_tostr(const list64_t list[], uint64_t val, char **str, char delim);

#endif
