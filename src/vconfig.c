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
#include <unistd.h>
#include <vserver.h>

#include "tools.h"
#include "vconfig.h"

#define NAME  "vcontext"
#define DESCR "Context Manager"

#define SHORT_OPTS "GSk:n:"

struct commands {
	bool get;
	bool set;
};

struct options {
	char *name;
	char *key;
};

static inline
void cmd_help()
{
	printf("Usage: %s <command> <opts>*\n"
	       "\n"
	       "Available commands:\n"
	       "    -G            Get configuration value\n"
	       "    -S            Set configuration value\n"
	       "\n"
	       "Available options:\n"
	       "    -n <name>     VServer name\n"
	       "    -k <key>      Configuration key\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct commands cmds = {
		.get = false,
		.set = false,
	};
	
	struct options opts = {
		.name = NULL,
		.key  = NULL,
	};
	
	int c;
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'G':
				cmds.get = true;
				break;
			
			case 'S':
				cmds.set = true;
				break;
			
			case 'n':
				opts.name = optarg;
				break;
			
			case 'k':
				opts.key = optarg;
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	if (opts.name == NULL)
		EXIT("<name> missing", EXIT_USAGE);
	
	if (opts.key == NULL)
		EXIT("<key> missing", EXIT_USAGE);
	
	
	if (cmds.get) {
		if (vconfig_isbool(opts.key) == 0) {
			bool buf = vconfig_get_bool(opts.name, opts.key);
			
			if (buf == true)
				printf("true\n");
			else
				printf("false\n");
		}
		
		if (vconfig_isint(opts.key) == 0) {
			int buf = vconfig_get_int(opts.name, opts.key);
			printf("%d\n", buf);
		}
		
		if (vconfig_isstr(opts.key) == 0) {
			char *buf = vconfig_get_str(opts.name, opts.key);
			printf("%s\n", buf);
			free(buf);
		}
	}
	
	if (cmds.set) {
		return -1;
	}
	
	return EXIT_SUCCESS;
}
