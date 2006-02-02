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
#define LIST32_START(LIST) static const list32_t LIST[] = {
#define LIST32_NODE(PREFIX, NAME) { #NAME, PREFIX ## _ ## NAME },
#define LIST32_END { NULL, 0 } };

#define LIST64_START(LIST) static const list64_t LIST[] = {
#define LIST64_NODE(PREFIX, NAME) { #NAME, PREFIX ## _ ## NAME },
#define LIST64_END { NULL, 0 } };

/* list methods */
uint32_t list32_getval(char *key, const list32_t list[]);
uint64_t list64_getval(char *key, const list64_t list[]);
char *list32_getkey(uint32_t val, const list32_t list[]);
char *list64_getkey(uint32_t val, const list64_t list[]);
void list32_parse(char *str, const list32_t list[],
                  uint32_t *flags, uint32_t *mask,
                  char clmod, char delim);
void list64_parse(char *str, const list64_t list[],
                  uint64_t *flags, uint64_t *mask,
                  char clmod, char delim);
char *list32_tostr(uint32_t val, const list32_t list[], char delim);
char *list64_tostr(uint64_t val, const list64_t *list, char delim);

#endif
