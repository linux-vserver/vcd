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

#include "printf.h"
#include "tools.h"
#include "vconfig.h"

#define NAME  "vcontext"
#define DESCR "Context Manager"

#define SHORT_OPTS "GLSk:n:"

typedef enum { VCFG_NONE, VCFG_LIST, VCFG_GET, VCFG_SET } command_t;

struct options {
	GLOBAL_OPTS;
	command_t cmd;
	char *name;
	char *key;
};

static inline
void cmd_help()
{
	vu_printf("Usage: %s <command> <opts>*\n"
	       "\n"
	       "Available commands:\n"
	       "    -G            Get configuration value\n"
	       "    -L            List valid configuration keys\n"
	       "    -S            Set configuration value\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -n <name>     VServer name\n"
	       "    -k <key>      Configuration key\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.cmd  = VCFG_NONE,
		.name = NULL,
		.key  = NULL,
	};
	
	int c;
	
	DEBUGF("%s: starting ...\n", NAME);
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'G':
				if (opts.cmd != VCFG_NONE)
					cmd_help();
				else
					opts.cmd = VCFG_GET;
				break;
			
			case 'L':
				if (opts.cmd != VCFG_NONE)
					cmd_help();
				else
					opts.cmd = VCFG_LIST;
				break;
			
			case 'S':
				if (opts.cmd != VCFG_NONE)
					cmd_help();
				else
					opts.cmd = VCFG_SET;
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
	
	switch (opts.cmd) {
		case VCFG_GET: {
			if (opts.name == NULL)
				EXIT("<name> missing", EXIT_USAGE);
		
			if (opts.key == NULL)
				EXIT("<key> missing", EXIT_USAGE);
		
			if (vconfig_isbool(opts.key) == 0) {
				int buf = vconfig_get_bool(opts.name, opts.key);
			
				if (buf != -1) {
					if (buf == 1)
						vu_printf("true\n");
					else
						vu_printf("false\n");
				}
			}
		
			if (vconfig_isint(opts.key) == 0) {
				int buf = vconfig_get_int(opts.name, opts.key);
				if (buf != -1)
					vu_printf("%d\n", buf);
			}
		
			if (vconfig_isstr(opts.key) == 0) {
				char *buf = vconfig_get_str(opts.name, opts.key);
			
				if (buf != NULL) {
					vu_printf("%s\n", buf);
					free(buf);
				}
			}
		
			if (vconfig_islist(opts.key) == 0) {
				char *buf = vconfig_get_list(opts.name, opts.key);
			
				if (buf != NULL) {
					vu_printf("%s\n", buf);
					free(buf);
				}
			}
			break;
		}
		case VCFG_LIST: {
			vconfig_print_nodes();
			break;
		}
		case VCFG_SET: {
			vu_printf("SET command not implemented\n");
			return -1;
			break;
		}
		default:
			cmd_help();
	}
	
	return EXIT_SUCCESS;
}
