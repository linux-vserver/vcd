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

#define NAME  "vcontext"
#define DESCR "Context Manager"

#define SHORT_OPTS "CIMx:f:"

struct commands {
	bool create;
	bool info;
	bool migrate;
};

struct options {
	xid_t xid;
	list_t *flags;
};

static inline
void cmd_help()
{
	printf("Usage: %s <command> <opts>* -- <program> <args>*\n"
	       "\n"
	       "Available commands:\n"
	       "    -C            Create a new security context\n"
	       "    -I            Get information about a context\n"
	       "    -M            Migrate to an existing context\n"
	       "\n"
	       "Available options:\n"
	       "    -x <xid>      Context ID\n"
	       "    -f <list>     Set flags described in <list>\n"
	       "\n"
	       "Flag list format string:\n"
	       "    <list> = [~]<flag>,[~]<flag>,...\n"
	       "\n"
	       "    See 'vflags -L' for available flags\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct commands cmds = {
		.create   = false,
		.info     = false,
		.migrate  = false,
	};
	
	struct options opts = {
		.xid   = (~0U),
		.flags = 0,
	};
	
	/* init syscall data */
	struct vx_create_flags create_flags = {
		.flags = 0,
	};
	
	struct vx_info info;
	
	/* init flags list */
	list_t *cp = cflags_list_init();
	
	int c;
	const char delim = ','; // list delimiter
	const char clmod = '~'; // clear flag modifier
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'C':
				cmds.create = true;
				break;
			
			case 'I':
				cmds.info = true;
				break;
			
			case 'M':
				cmds.migrate = true;
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			case 'f':
				opts.flags = list_parse_list(optarg, delim);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	if (cmds.create) {
		if (opts.flags == 0)
			goto create;
		
		list_link_t link = {
			.p = cp,
			.d = opts.flags,
		};
		
		/* validate descending list */
		if (list_validate_flag(&link, clmod) == -1)
			PEXIT("List validation failed", EXIT_USAGE);
		
		/* vx_create_flags has no mask member
		 * so we create a dumb one */
		uint64_t mask = 0;
		
		/* convert given descending list to flags using the pristine copy */
		list_list2flags(&link, clmod, &create_flags.flags, &mask);
		
create:
		/* syscall */
		if (vx_create(opts.xid, &create_flags) == -1)
			PEXIT("Failed to create context", EXIT_COMMAND);
		
		goto load;
	}
	
	if (cmds.migrate) {
		/* syscall */
		if (vx_migrate(opts.xid) == -1)
			PEXIT("Failed to migrate to context", EXIT_COMMAND);
		
		goto load;
	}
	
	if (cmds.info) {
		/* syscall */
		if (vx_get_info(opts.xid, &info) == -1)
			PEXIT("Failed to get context information", EXIT_COMMAND);
		
		printf("Context ID: %d\n", info.xid);
		printf("Init PID: %d\n", info.initpid);
		
		goto out;
	}
	
	cmd_help();
	
load:
	if (argc > optind)
		execvp(argv[optind], argv+optind);
	
out:
	exit(EXIT_SUCCESS);
}
