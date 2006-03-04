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
#include <vserver.h>

#include "printf.h"
#include "tools.h"
#include "vlist.h"

#define NAME  "vnflags"
#define DESCR "Network Context Flag/Capability Manager"

#define SHORT_OPTS "SGLn:c:f:m"

typedef enum { VNFLAG_NONE, VNFLAG_GET, VNFLAG_SET, VNFLAG_LIST } command_t;

struct options {
	GLOBAL_OPTS;
	command_t cmd;
	nid_t nid;
	bool compact;
	list_t *caps;
	list_t *flags;
};

static inline
void cmd_help()
{
	vu_printf("Usage: %s <command> <opts>*\n"
	       "\n"
	       "Available commands:\n"
	       "    -S            Set context capabilities/flags\n"
	       "    -G            Get context capabilities/flags\n"
	       "    -L            List available capabilities/flags\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -n <nid>      Network context ID\n"
	       "    -c <list>     Set capabilities described in <list>\n"
	       "    -f <list>     Set flags described in <list>\n"
	       "\n"
	       "Flag list format string:\n"
	       "    <list> = [~]<flag>,[~]<flag>,...\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.cmd   = VNFLAG_NONE,
		.nid   = 0,
		.caps  = 0,
		.flags = 0,
		.compact = 0,
	};
	
	/* init syscall data */
	struct nx_flags flags = {
		.flags = 0,
		.mask  = 0,
	};
	
	struct nx_caps caps = {
		.caps = 0,
		.mask = 0,
	};
	
	/* init capability lists */
	list_t *cp = list_alloc(0);
	list_t *fp = nflags_list_init();
	
	int c;
	const char delim = ','; // list delimiter
	const char clmod = '~'; // clear flag modifier
	
	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'S':
				if (opts.cmd != VNFLAG_NONE)
					cmd_help();
				else
					opts.cmd = VNFLAG_SET;
				break;
			
			case 'G':
				if (opts.cmd != VNFLAG_NONE)
					cmd_help();
				else
					opts.cmd = VNFLAG_GET;
				break;
			
			case 'L':
				if (opts.cmd != VNFLAG_NONE)
					cmd_help();
				else
					opts.cmd = VNFLAG_LIST;
				break;
			
			case 'n':
				opts.nid = (nid_t) atoi(optarg);
				break;
			
			case 'c':
				opts.caps = list_parse_list(optarg, delim);
				break;
			
			case 'f':
				opts.flags = list_parse_list(optarg, delim);
				break;
				
			case 'm':
				opts.compact = 1;
				break;
				
			DEFAULT_GETOPT
		}
	}

	switch (opts.cmd) {
		case VNFLAG_GET:
			/* syscall */
			if (nx_get_caps(opts.nid, &caps) == -1)
				PEXIT("Failed to get context capabilities", EXIT_COMMAND);
			
			if (nx_get_flags(opts.nid, &flags) == -1)
				PEXIT("Failed to get context flags", EXIT_COMMAND);
			
			/* iterate through each list and print matching keys */
			if (opts.compact) {
				int b = 0;
				vu_printf("NCAPS =");
				list_foreach(cp, i)
					if (caps.caps & *(uint64_t*)(cp->node+i)->data)
						vu_printf("%s%s", b++ ? "," : " ", (char *)(cp->node+i)->key);
				vu_printf("\nFLAGS =");
				b = 0;
				list_foreach(fp, i)
					if (flags.flags & *(uint64_t*)(fp->node+i)->data)
						vu_printf("%s%s", b++ ? "," : " ", (char *)(fp->node+i)->key);
				vu_printf("\n");
			} else {
				list_foreach(cp, i) {
					if (caps.caps & *(uint64_t*)(cp->node+i)->data)
						vu_printf("C: %s\n", (char *)(cp->node+i)->key);
				}
			
				list_foreach(fp, i) {
					if (flags.flags & *(uint64_t*)(fp->node+i)->data)
						vu_printf("F: %s\n", (char *)(fp->node+i)->key);
				}
			}
			break;
		
		case VNFLAG_SET: {
			if (opts.caps == 0)
				goto nflags;
			
			list_link_t clink = {
				.p = cp,
				.d = opts.caps,
			};
			
			/* validate descending lists */
			if (list_validate_flag(&clink, clmod) == -1)
				PEXIT("List validation failed", EXIT_USAGE);
			
			/* convert given descending lists to flags using the pristine copies */
			list_list2flags(&clink, clmod, &caps.caps, &caps.mask);
			
			/* syscall */
			if (nx_set_caps(opts.nid, &caps) == -1)
				PEXIT("Failed to set network context capabilities", EXIT_COMMAND);
			
nflags:
			if (opts.flags == 0)
				EXIT("No capabilities/flags specified", EXIT_USAGE);
			
			list_link_t flink = {
				.p = fp,
				.d = opts.flags,
			};
			
			/* validate descending list */
			if (list_validate_flag(&flink, clmod) == -1)
				PEXIT("List validation failed", EXIT_USAGE);
			
			/* convert given descending list to flags using the pristine copy */
			list_list2flags(&flink, clmod, &flags.flags, &flags.mask);
			
			/* syscall */
			if (nx_set_flags(opts.nid, &flags) == -1)
				PEXIT("Failed to set context flags", EXIT_COMMAND);
			break;
		}
		case VNFLAG_LIST:
			/* iterate through each list and print all keys */
			list_foreach(cp, i)
				vu_printf("C: %s\n", (char *)(cp->node+i)->key);
			
			list_foreach(fp, i)
				vu_printf("F: %s\n", (char *)(fp->node+i)->key);
			break;
		
		default:
			cmd_help();
	}
	
	exit(EXIT_SUCCESS);
}
