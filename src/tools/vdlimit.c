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
#include <string.h>
#include <strings.h>
#include <vserver.h>

#ifndef __user
#define __user
#endif

#ifdef __user
#undef __user
#endif

#include <lucid/printf.h>
#include "tools.h"
#include "vlist.h"

#define NAME  "vdlimit"
#define DESCR "Disk Limit Manager"

#define SHORT_OPTS "ARSGx:l:"

typedef enum { VDL_NONE, VDL_ADD, VDL_REM, VDL_SET, VDL_GET } command_t;

struct options {
	GLOBAL_OPTS;
	command_t cmd;
	xid_t xid;
	list_t *limit;
};

static inline
void cmd_help()
{
	_lucid_printf("Usage: %s <command> <opts>* <mountpoint>\n"
	       "\n"
	       "Available commands:\n"
	       "    -A            Add a disk limit entry\n"
	       "    -R            Remove a disk limit entry\n"
	       "    -S            Set disk limit\n"
	       "    -G            Get disk limit\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -x <xid>      Context ID\n"
	       "    -l <limit>    Set limit described in <limit>\n"
	       "\n"
	       "Disk Limit format string:\n"
	       "    <limit> = <SU>,<SM>,<IU>,<IM>,<RR> where\n"
	       "               - SU is the used space in kbytes,\n"
	       "               - SM is the maximum space in kbytes,\n"
	       "               - IU is the used inode count,\n"
	       "               - IM is the maximum inode count, and\n"
	       "               - RR is reserved for root in percent.\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.cmd = VDL_NONE,
		.xid = 0,
		.limit = 0,
	};
	
	/* init syscall data */
	struct vx_dlimit_base base = {
		.filename = 0,
		.flags    = 0,
	};
	
	/* TODO: CDLIM_KEEP is 64bit, whereas dlimit struct contains 32bit members */
	struct vx_dlimit dlimit = {
		.filename = 0,
		.space_used = CDLIM_KEEP,
		.space_total = CDLIM_KEEP,
		.inodes_used = CDLIM_KEEP,
		.inodes_total = CDLIM_KEEP,
		.reserved = CDLIM_KEEP,
	};
	
	int c;
	const char delim = ','; // list delimiter
	
	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'A':
				if (opts.cmd != VDL_NONE)
					cmd_help();
				else
					opts.cmd = VDL_ADD;
				break;
			
			case 'R':
				if (opts.cmd != VDL_NONE)
					cmd_help();
				else
					opts.cmd = VDL_REM;
				break;
			
			case 'S':
				if (opts.cmd != VDL_NONE)
					cmd_help();
				else
					opts.cmd = VDL_SET;
				break;
			
			case 'G':
				if (opts.cmd != VDL_NONE)
					cmd_help();
				else
					opts.cmd = VDL_GET;
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			case 'l':
				opts.limit = list_parse_list(optarg, delim);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	/* get disk limit filename */
	base.filename = dlimit.filename = argv[optind];
	base.flags    = dlimit.flags    = 0;

	switch (opts.cmd) {
		case VDL_ADD:
			/* syscall */
			if (vx_add_dlimit(opts.xid, &base) == -1)
				PEXIT("Failed to add disk limit entry", EXIT_COMMAND);
			break;
			
		case VDL_REM:
			/* syscall */
			if (vx_rem_dlimit(opts.xid, &base) == -1)
				PEXIT("Failed to remove disk limit entry", EXIT_COMMAND);
			break;
			
		case VDL_SET: {
			/* let's make a pointer to prevent unreadable code */
			list_node_t *lnode = (opts.limit)->node;
			
#define SETLIMIT(LIMIT) { \
	if (strlen(lnode->key) != 0) { \
		if (strcasecmp(lnode->key, "inf") == 0) \
			LIMIT = CDLIM_INFINITY; \
		else LIMIT = atoi(lnode->key); \
	} \
	lnode++; \
}
			/* set disk limit values */
			SETLIMIT(dlimit.space_used)
			SETLIMIT(dlimit.space_total)
			SETLIMIT(dlimit.inodes_used)
			SETLIMIT(dlimit.inodes_total)
			SETLIMIT(dlimit.reserved)
#undef SETLIMIT

			/* syscall */
			if (vx_set_dlimit(opts.xid, &dlimit) == -1)
				PEXIT("Failed to set disk limit", EXIT_COMMAND);
			
			break;
		}
		case VDL_GET: {
			/* syscall */
			if (vx_get_dlimit(opts.xid, &dlimit) == -1)
				PEXIT("Failed to get disk limit", EXIT_COMMAND);
			
#define PRINTLIMIT(LIMIT, SUFFIX) { \
	if (LIMIT == CDLIM_INFINITY) \
		_lucid_printf("inf" SUFFIX); \
	else \
		_lucid_printf("%u" SUFFIX, LIMIT); \
}
			/* print list parser compliant limit string */
			PRINTLIMIT(dlimit.space_used, ",")
			PRINTLIMIT(dlimit.space_total, ",")
			PRINTLIMIT(dlimit.inodes_used, ",")
			PRINTLIMIT(dlimit.inodes_total, ",")
			PRINTLIMIT(dlimit.reserved, "\n")
#undef PRINTLIMIT
			break;
		}
		default:
			cmd_help();
	}

	exit(EXIT_SUCCESS);
}
