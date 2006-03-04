/***************************************************************************
 *   Copyright 2005 The libvserver Team                                    *
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

#include "vlist.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

list_t *list_alloc(size_t n)
{
	list_t *list      = malloc(sizeof(list_t));
	list_node_t *node = malloc(sizeof(list_node_t)*n);
	
	list->node = node;
	list->n    = n;
	
	return list;
}

void list_dealloc(list_t *list)
{
	unsigned int i;
	for (i = 0; i < list->n; i++) {
		free((list->node+i)->key);
		free((list->node+i)->data);
	}
	
	free(list->node);
	free(list);
}

char *list_key_alloc(char *value)
{
	size_t keylen = strlen(value);
	
	char *key = malloc(keylen+1);
	memcpy(key, value, keylen);
	key[keylen] = '\0';
	
	return key;
}

void list_set(list_node_t *node, char *key, void *data)
{
	node->key  = key;
	node->data = data;
}

size_t list_ntokens(const char *str, const char delim)
{
	size_t ntokens = strlen(str) ? 1 : 0;
	
	while ((str = strchr(str, delim))) {
		str++; /* Skip delimiter */
		ntokens++;
	}
	
	return ntokens;
}

char *list_parse(const char **str, const char delim)
{
	const char *ptr  = strchr(*str, delim); /* get pointer to next delim */
	size_t len       = 0;
		
	len = ptr ? (size_t) (ptr - (*str)) : strlen(*str); /* get token legth */
	
	if (len == 0) { /* empty token */
		*str = ++(*str);
		return "";
	}
	
	char *token = malloc(len+1);
	memcpy(token, *str, len);
	token[len] = '\0';
	
	*str = ++ptr;
	
	return token;
}

list_t *list_parse_hash(const char *str, const char delim, const char kvdelim)
{
	list_t *list = list_alloc(list_ntokens(str, delim));
	
	unsigned int i;
	for(i = 0; i < list->n; i++) {
		char *token = list_parse(&str, delim);
		char *ptr   = strchr(token, kvdelim);
		
		if (ptr == NULL) { /* invalid hash token: kvdelim not found */
			errno = EINVAL;
			return NULL;
		}
		
		size_t keylen = (ptr - token);
		size_t vallen = strlen(++ptr);
		
		if (keylen == 0) { /* key may not be emtpy! */
			errno = EINVAL;
			return NULL;
		}
		
		char *key = malloc(keylen+1);
		char *val = malloc(vallen+1);
		
		memcpy(key, token, keylen);
		key[keylen] = '\0';
		
		memcpy(val, ptr, vallen);
		val[vallen] = '\0';
		
		list_set(list->node+i, key, val);
		
		free(token);
	}
	
	return list;
}

list_t *list_parse_list(const char *str, const char delim)
{
	list_t *list = list_alloc(list_ntokens(str, delim));
	
	unsigned int i;
	for(i = 0; i < list->n; i++) {
		list_set(list->node+i,
		         list_parse(&str, delim),
		         NULL);
	}
	
	return list;
}

list_node_t *list_search(list_t *list, char *key)
{
	unsigned int i;
	for (i = 0; i < list->n; i++) {
		if(strcasecmp(key, (list->node+i)->key) == 0)
			return list->node+i;
	}
	
	return NULL;
}

int list_validate_flag(list_link_t *link, const char clmod)
{
	unsigned int i;
	for (i = 0; i < link->d->n; i++) {
		list_node_t *ptr = link->d->node+i;
		int skip         = *ptr->key == clmod ? 1 : 0;
		
		if (list_search(link->p, ptr->key+skip) == NULL) {
			errno = EINVAL;
			return -1;
		}
	}
	
	return 0;
}

int list_validate(list_link_t *link)
{
	const char clmod = '\0';
	return list_validate_flag(link, clmod);
}

void list_list2flags(list_link_t *link, const char clmod,
                     uint64_t *flags, uint64_t *mask)
{
	unsigned int i;
	for (i = 0; i < link->d->n; i++) {
		char *key;
		bool clear = false;
		
		list_node_t *ptr = link->d->node+i;
		
		if (*(const char *)ptr->key == clmod) {
			clear = true;
			key   = ptr->key+1;
		}
		else {
			key   = ptr->key;
		}
		
		list_node_t *node = list_search(link->p, key);
		
		if (!node) break;
		
		uint64_t num_flag = *(uint64_t *)node->data;
		
		if (clear) {
			*flags &= ~num_flag; /* clear flag */
			*mask  |=  num_flag; /* set mask */
		}
		else {
			*flags |=  num_flag; /* set flag */
			*mask  |=  num_flag; /* set mask */
		}
	}
}
