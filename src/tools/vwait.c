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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdlib.h>
#include <vserver.h>

#include "printf.h"
#include "tools.h"

#define NAME  "vwait"
#define DESCR "Kill context processes"

#define SHORT_OPTS "x:"

struct options {
	xid_t xid;
};

static inline
void cmd_help()
{
 vu_printf("Usage: %s <opts>*\n"
	       "\n"
	       "Available options:\n"
	       "    -x <xid>      Context ID\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct options opts = {
		.xid = 0,
	};
	
	/* init syscall data */
	struct vx_wait_opts wait_opts = {
		.a = 0,
		.b = 0,
	};
	
	int c;
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	/* show help message */
	if (argc == 1) {
		cmd_help();
		exit(EXIT_SUCCESS);
	}
	
	if (opts.xid == 0)
		EXIT("No xid given", EXIT_USAGE);
	
	/* syscall */
	if (vx_wait(opts.xid, &wait_opts) == -1)
		PEXIT("Failed to wait for context", EXIT_COMMAND);
	
	exit(EXIT_SUCCESS);
}
