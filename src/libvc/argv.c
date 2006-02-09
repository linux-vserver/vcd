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

#include "vc.h"

char **vc_str_to_argv(char *str, int *argc)
{
	if (strlen(str) < 1)
		return NULL;
	
	int i = 0, len;
	char *buf = strdup(str), *p;
	
	int ac = 0;
	char **av;
	
	/* count arguments */
	for (p = strtok(buf, " "); p != NULL; ac++)
		p = strtok(NULL, " ");
	
	free(buf);
	
	av = (char **) calloc(ac, sizeof(char *));
	
	for (p = strtok(str, " "); p != NULL && i < ac; i++) {
		len = strlen(p) + 1;
		
		av[i] = malloc(len);
		memset(av[i], '\0', len);
		memcpy(av[i], p, len-1);
		
		p = strtok(NULL, " ");
	}
	
	*argc = ac;
	
	return av;
}

void vc_argv_free(int argc, char **argv)
{
	int i;
	
	for (i = 0; i < argc; i++)
		free(argv[i]);
	
	free(argv);
}
