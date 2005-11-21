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
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <vserver.h>

#include <linux/vserver/context.h>

#include "tools.h"

#define NAME  "vexec"
#define DESCR "Execute in vserver context"

#define SHORT_OPTS "cfix:"

struct options {
	bool chroot;
	bool fork;
	bool init;
	xid_t xid;
};

static inline
void cmd_help()
{
	printf("Usage: %s <opts>* -- <command> <args>*\n"
	       "\n"
	       "Available options:\n"
	       "    -c            chroot to current working directory\n"
	       "    -f            Fork into background\n"
	       "    -i            Set init PID\n"
	       "    -x <xid>      Context ID\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct options opts = {
		.chroot = false,
		.fork   = false,
		.init   = false,
		.xid    = 0,
	};
	
	/* init syscall data */
	struct vx_flags flags = {
		.flags = 0,
		.mask  = VXF_PERSISTANT|VXF_STATE_SETUP,
	};
	
	int c;
	pid_t pid = 0; /* sys_clone */
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'c':
				opts.chroot = true;
				break;
			
			case 'f':
				opts.fork = true;
				break;
			
			case 'i':
				opts.init = true;
				flags.flags |= VXF_INFO_INIT;
				flags.mask  |= VXF_INFO_INIT;
				flags.mask  |= VXF_STATE_INIT;
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	/* show help message */
	if (argc == 1)
		cmd_help();
	
	if (opts.xid == 0)
		EXIT("No xid given", EXIT_USAGE);
	
	if (argc <= optind)
		EXIT("No init command given", EXIT_USAGE);
	
	if (opts.fork) {
		pid = sys_clone(CLONE_VFORK, 0);
	}
	
	switch(pid) {
		case -1:
			PEXIT("Failed to clone process", EXIT_COMMAND);
		
		case 0:
			if (opts.xid == 1)
				goto migrate;
			
			/* syscall */
			if (!opts.init)
				if (vx_set_flags(opts.xid, &flags) == -1)
					PEXIT("Failed to set context flags", EXIT_COMMAND);
			
migrate:
			if (vx_migrate(opts.xid) == -1)
				PEXIT("Failed to migrate to context", EXIT_COMMAND);
			
			if (opts.init)
				if (vx_set_flags(opts.xid, &flags) == -1)
					PEXIT("Failed to set context flags", EXIT_COMMAND);
			
			/* chroot to cwd */
			if (opts.chroot) {
				if (chdir(".") == -1)
					PEXIT("Failed to chdir to cwd", EXIT_COMMAND);
				
				if (chroot(".") == -1)
					PEXIT("Failed to chroot to cwd", EXIT_COMMAND);
			}
			
			if(execvp(argv[optind], argv+optind) == -1)
				PEXIT("Failed to start init", EXIT_COMMAND);
		
		default:
			exit(EXIT_SUCCESS);
	}
}
