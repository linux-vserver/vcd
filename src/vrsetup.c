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

#include "tools.h"

#define NAME  "vrsetup"
#define DESCR "Virtual Root Device Manager"

#define SHORT_OPTS "CDIS"

#define VROOT_SET_DEV 0x5600
#define VROOT_CLR_DEV 0x5601
#define VROOT_INC_USE 0x56FE
#define VROOT_DEC_USE 0x56FF

struct commands {
	bool clear;
	bool decr;
	bool incr;
	bool setup;
};

struct options {
	char *root;
	char *realroot;
};

static inline
void cmd_help()
{
	printf("Usage: %s <command> <rootdev> [<realrootdev>]\n"
	       "\n"
	       "Available commands:\n"
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
	struct commands cmds = {
		.clear = false,
		.decr  = false,
		.incr  = false,
		.setup = false,
	};
	
	struct options opts = {
		.root     = NULL,
		.realroot = NULL,
	};
	
	int c;
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'C':
				cmds.clear = true;
				break;
			
			case 'D':
				cmds.decr = true;
				break;
			
			case 'I':
				cmds.incr = true;
				break;
			
			case 'S':
				cmds.setup = true;
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	/* check for root dev */
	if (argc < optind + 1)
		EXIT("<rootdev> missing", EXIT_USAGE);
	
	opts.root = argv[optind];
	
	/* check for real root dev */
	if (cmds.setup && argc < optind + 2)
		EXIT("<realrootdev> missing", EXIT_COMMAND)
	
	if (cmds.setup)
		opts.realroot = argv[optind+1];
	
	int rootfd = open(opts.root, O_RDONLY, 0);
	
	if (rootfd == -1)
		PEXIT("Failed to open <rootdev>", EXIT_COMMAND);
	
	if (cmds.clear) {
		if (ioctl(rootfd, VROOT_CLR_DEV, 0) == -1)
			PEXIT("Failed to clear device", EXIT_COMMAND);
	}
	
	else if (cmds.decr) {
		if (ioctl(rootfd, VROOT_DEC_USE, 0) == -1)
			PEXIT("Failed to decrement device usage", EXIT_COMMAND);
	}
	
	else if (cmds.incr) {
		if (ioctl(rootfd, VROOT_INC_USE, 0) == -1)
			PEXIT("Failed to increment device usage", EXIT_COMMAND);
	}
	
	else if (cmds.setup) {
		int realrootfd = open(opts.realroot, O_RDONLY, 0);
		
		if (realrootfd == -1)
			PEXIT("Failed to open <realrootdev>", EXIT_COMMAND);
		
		if (ioctl(rootfd, VROOT_SET_DEV, (void *)realrootfd) == -1)
			PEXIT("Failed to decrement device usage", EXIT_COMMAND);
		
		close(realrootfd);
	}
	
	close(rootfd);
	
	exit(EXIT_SUCCESS);
}
