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

#ifndef LIBVCONFIG_HAVE_BACKEND
#define LIBVCONFIG_HAVE_BACKEND

#include <stdbool.h>

#include "vconfig.h"

/* bool methods */
bool vconfig_get_bool(char *name, char *key)
{
	return false;
}

int vconfig_set_bool(char *name, char *key, bool value)
{
	return -1;
}

/* integer methods */
int vconfig_get_int(char *name, char *key)
{
	return -1;
}

int vconfig_set_int(char *name, char *key, int value)
{
	return -1;
}

/* string methods */
char *vconfig_get_str(char *name, char *key)
{
	return NULL;
}

int vconfig_set_str(char *name, char *key, char *value)
{
	return -1;
}

#endif
