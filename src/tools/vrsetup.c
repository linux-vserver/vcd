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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <vserver.h>

#include "printf.h"
#include "tools.h"

#define NAME  "vrsetup"
#define DESCR "Virtual Root Device Manager"

#define SHORT_OPTS "CDIS"

#define VROOT_SET_DEV 0x5600
#define VROOT_CLR_DEV 0x5601
#define VROOT_INC_USE 0x56FE
#define VROOT_DEC_USE 0x56FF

typedef enum { VRSET_NONE, VRSET_DECR, VRSET_INCR, VRSET_SETUP, VRSET_CLEAR } command_t;

struct options {
	GLOBAL_OPTS;
	command_t cmd;
	char *root;
	char *realroot;
};

static inline
void cmd_help()
{
	vu_printf("Usage: %s <command> <rootdev> [<realrootdev>]\n"
	       "\n"
	       "Available commands:\n"
	       GLOBAL_HELP
	       "    -C            Clear device\n"
	       "    -D            Decrement device usage\n"
	       "    -I            Increment device usage\n"
	       "    -S            Setup device\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.cmd      = VRSET_NONE,
		.root     = NULL,
		.realroot = NULL,
	};
	
	int c;
	
	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'C':
				if (opts.cmd != VRSET_NONE)
					cmd_help();
				else
					opts.cmd = VRSET_CLEAR;
				break;
			
			case 'D':
				if (opts.cmd != VRSET_NONE)
					cmd_help();
				else
					opts.cmd = VRSET_DECR;
				break;
			
			case 'I':
				if (opts.cmd != VRSET_NONE)
					cmd_help();
				else
					opts.cmd = VRSET_INCR;
				break;
			
			case 'S':
				if (opts.cmd != VRSET_NONE)
					cmd_help();
				else
					opts.cmd = VRSET_SETUP;
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	/* check for root dev */
	if (argc < optind + 1)
		EXIT("<rootdev> missing", EXIT_USAGE);
	
	opts.root = argv[optind];
	int rootfd = open(opts.root, O_RDONLY, 0);
	if (rootfd == -1)
		PEXIT("Failed to open <rootdev>", EXIT_COMMAND);

	switch (opts.cmd) {
		case VRSET_SETUP: {
			if (argc < optind + 2)
				EXIT("<realrootdev> missing", EXIT_COMMAND);
			opts.realroot = argv[optind+1];
			int realrootfd = open(opts.realroot, O_RDONLY, 0);
			
			if (realrootfd == -1)
				PEXIT("Failed to open <realrootdev>", EXIT_COMMAND);
			
			if (ioctl(rootfd, VROOT_SET_DEV, (void *)realrootfd) == -1)
				PEXIT("Failed to decrement device usage", EXIT_COMMAND);
			
			close(realrootfd);
			break;
		}
		case VRSET_DECR:
			if (ioctl(rootfd, VROOT_DEC_USE, 0) == -1)
				PEXIT("Failed to decrement device usage", EXIT_COMMAND);
			break;

		case VRSET_INCR:
			if (ioctl(rootfd, VROOT_INC_USE, 0) == -1)
				PEXIT("Failed to increment device usage", EXIT_COMMAND);
			break;
		
		case VRSET_CLEAR:
			if (ioctl(rootfd, VROOT_CLR_DEV, 0) == -1)
				PEXIT("Failed to clear device", EXIT_COMMAND);
			break;

		default:
			cmd_help();
	}
	
	close(rootfd);
	exit(EXIT_SUCCESS);
}
