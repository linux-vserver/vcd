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

#define NAME  "vflags"
#define DESCR "Context Flag/Capability Manager"

#define SHORT_OPTS "SGLx:b:c:f:"

struct commands {
	bool set;
	bool get;
	bool list;
};

struct options {
	xid_t xid;
	list_t *bcaps;
	list_t *ccaps;
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
	       "    -x <xid>      Context ID\n"
	       "    -b <list>     Set capabilities described in <list>\n"
	       "                  (You can only lower bcaps!)\n"
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
	struct commands cmds = {
		.set  = false,
		.get  = false,
		.list = false,
	};
	
	struct options opts = {
		.xid   = 0,
		.bcaps = 0,
		.ccaps = 0,
		.flags = 0,
	};
	
	/* init syscall data */
	struct vx_flags flags = {
		.flags = 0,
		.mask  = 0,
	};
	
	struct vx_caps caps = {
		.bcaps = -1,
		.bmask = -1,
		.ccaps = 0,
		.cmask = 0,
	};
	
	/* init capability lists */
	list_t *bp = bcaps_list_init();
	list_t *cp = ccaps_list_init();
	list_t *fp = cflags_list_init();
	
	int c;
	const char delim = ','; // list delimiter
	const char clmod = '~'; // clear flag modifier
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'S':
				cmds.set = true;
				break;
			
			case 'G':
				cmds.get = true;
				break;
			
			case 'L':
				cmds.list = true;
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			case 'b':
				opts.bcaps = list_parse_list(optarg, delim);
				break;
			
			case 'c':
				opts.ccaps = list_parse_list(optarg, delim);
				break;
			
			case 'f':
				opts.flags = list_parse_list(optarg, delim);
				break;
			
			DEFAULT_GETOPT
		}
	}

	if (cmds.set) {
		if (opts.bcaps == 0)
			goto ccaps;
		
		list_link_t blink = {
			.p = bp,
			.d = opts.bcaps,
		};
		
		/* validate descending list */
		if (list_validate_flag(&blink, clmod) == -1)
			PEXIT("List validation failed", EXIT_USAGE);
		
		/* convert given descending list to flags using the pristine copy */
		list_list2flags(&blink, clmod, &caps.bcaps, &caps.bmask);
		
ccaps:
		if (opts.ccaps == 0)
			goto cflags;
		
		list_link_t clink = {
			.p = cp,
			.d = opts.ccaps,
		};
		
		/* validate descending lists */
		if (list_validate_flag(&clink, clmod) == -1)
			PEXIT("List validation failed", EXIT_USAGE);
		
		/* convert given descending list to flags using the pristine copy */
		list_list2flags(&clink, clmod, &caps.ccaps, &caps.cmask);
		
		/* syscall */
		if (vx_set_caps(opts.xid, &caps) == -1)
			PEXIT("Failed to set context capabilities", EXIT_COMMAND);
		
cflags:
		if (opts.flags == 0)
			goto out;
		
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
		if (vx_set_flags(opts.xid, &flags) == -1)
			PEXIT("Failed to set context flags", EXIT_COMMAND);
		
		goto out;
	}
	
	if (cmds.get) {
		/* syscall */
		if (vx_get_caps(opts.xid, &caps) == -1)
			PEXIT("Failed to get context capabilities", EXIT_COMMAND);
		
		if (vx_get_flags(opts.xid, &flags) == -1)
			PEXIT("Failed to get context flags", EXIT_COMMAND);
		
		/* iterate through each list and print matching keys */
		list_foreach(bp, i) {
			if (caps.bcaps & *(uint64_t*)(bp->node+i)->data)
			 vu_printf("B: %s\n", (char *)(bp->node+i)->key);
		}
		
		list_foreach(cp, i) {
			if (caps.ccaps & *(uint64_t*)(cp->node+i)->data)
			 vu_printf("C: %s\n", (char *)(cp->node+i)->key);
		}
		
		list_foreach(fp, i) {
			if (flags.flags & *(uint64_t*)(fp->node+i)->data)
			 vu_printf("F: %s\n", (char *)(fp->node+i)->key);
		}
		
		goto out;
	}
	
	if (cmds.list) {
		/* iterate through each list and print all keys */
		list_foreach(bp, i)
		 vu_printf("B: %s\n", (char *)(bp->node+i)->key);
		
		list_foreach(cp, i)
		 vu_printf("C: %s\n", (char *)(cp->node+i)->key);
		
		list_foreach(fp, i)
		 vu_printf("F: %s\n", (char *)(fp->node+i)->key);
		
		goto out;
	}
	
	cmd_help();
	
out:
	exit(EXIT_SUCCESS);
}
