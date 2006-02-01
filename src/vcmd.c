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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <libgen.h>

#include "printf.h"
#include "msg.h"

/* link all commands */
#include "commands/commands.h"
#include "commands/context.c"

static inline
void usage(int rc)
{
	vu_printf("Usage: vcmd <command> <opts>*\n"
	          "\n"
	          "Available commands:\n"
	          /* TODO: list commands */
	          "\n");
	exit(rc);
}

int main(int argc, char *argv[])
{
	char *cmd;
	
	/* check argv[0] */
	argv0 = cmd = basename(argv[0]);
	
	if (strcmp(cmd, "vcmd") == 0) {
		if (argc <= 1)
			usage(EXIT_FAILURE);
		
		cmd = basename(argv[1]);
		argv = &argv[1];
		argc--;
	}
	
	int i;
	
	/* try to run command */
	for (i = 0; commands[i].name; i++)
		if (strcmp(commands[i].name, cmd) == 0)
			return commands[i].func(argc, argv);
	
	/* none found */
	err("Unknown command: %s", cmd);
	
	return EXIT_FAILURE;
}
