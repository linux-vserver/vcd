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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdlib.h>
#include <vserver.h>

#include <lucid/printf.h>
#include "tools.h"

#define NAME  "vinfo"
#define DESCR "VServer information"

#define SHORT_OPTS "v"

struct options {
	GLOBAL_OPTS;
};

void cmd_help()
{
	_lucid_printf("Usage: %s <opts>\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct options opts = {
		GLOBAL_OPTS_INIT,
	};
	
	int c;
	
	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			DEFAULT_GETOPT
		}
	}
	
	int version;
	
	if((version = vs_get_version()) == -1)
		PEXIT("Failed to get VServer interface version", EXIT_COMMAND);
	
	_lucid_printf("%04x:%04x\n", (version>>16)&0xFFFF, version&0xFFFF);
	
	exit(EXIT_SUCCESS);
}
