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

#include <vserver.h>

/* main.c */
int vconfig_isbool(char *key);
int vconfig_isint(char *key);
int vconfig_isstr(char *key);
int vconfig_islist(char *key);
int vconfig_print_nodes(void);

int vconfig_get_xid(char *name, xid_t *xid);
int vconfig_get_name(xid_t xid, char **name);

/* backend api */
int vconfig_get_bool(char *name, char *key, int *value);
int vconfig_set_bool(char *name, char *key, int value);
int vconfig_get_int(char *name, char *key, int *value);
int vconfig_set_int(char *name, char *key, int value);
int vconfig_get_str(char *name, char *key, char **value);
int vconfig_set_str(char *name, char *key, char *value);
int vconfig_get_list(char *name, char *key, char **value);
int vconfig_set_list(char *name, char *key, char *value);
