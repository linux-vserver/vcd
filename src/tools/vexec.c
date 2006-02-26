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
#include <errno.h>

#include <linux/vserver/context.h>
#include <linux/vserver/network.h>

#include "printf.h"
#include "tools.h"

#define NAME  "vexec"
#define DESCR "Execute in vserver context"

#define SHORT_OPTS "cfin:x:"

struct options {
	bool chroot;
	bool fork;
	bool init;
	nid_t nid;
	xid_t xid;
};

static inline
void cmd_help()
{
	vu_printf("Usage: %s <opts>* -- <command> <args>*\n"
	       "\n"
	       "Available options:\n"
	       "    -c            chroot to current working directory\n"
	       "    -f            Fork into background\n"
	       "    -i            Set init PID\n"
	       "    -n <nid>      Network Context ID\n"
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
		.nid    = 0,
		.xid    = 0,
	};
	
	/* init syscall data */
	struct vx_flags flags = {
		.flags = 0,
		.mask  = VXF_STATE_SETUP,
	};
	
	struct nx_flags nflags = {
		.flags = 0,
		.mask  = NXF_STATE_SETUP,
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
			
			case 'n':
				opts.nid = (nid_t) atoi(optarg);
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
		EXIT("No command given", EXIT_USAGE);
	
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
			if (!opts.init) {
				if (opts.nid > 1)
					if (nx_set_flags(opts.nid, &nflags) == -1)
						PEXIT("Failed to set network context flags", EXIT_COMMAND);
				
				if (vx_set_flags(opts.xid, &flags) == -1)
					PEXIT("Failed to set context flags", EXIT_COMMAND);
			}
			
migrate:
			if (opts.nid > 1)
				if (nx_migrate(opts.nid) == -1)
					PEXIT("Failed to migrate to network context", EXIT_COMMAND);
			
			if (vx_migrate(opts.xid) == -1)
				PEXIT("Failed to migrate to context", EXIT_COMMAND);
			
			if (opts.init) {
				if (opts.nid > 1)
					if (nx_set_flags(opts.nid, &nflags) == -1)
						PEXIT("Failed to set network context flags", EXIT_COMMAND);
				
				if (vx_set_flags(opts.xid, &flags) == -1)
					PEXIT("Failed to set context flags", EXIT_COMMAND);
			}
			
			/* chroot to cwd */
			if (opts.chroot) {
				if (chdir(".") == -1)
					PEXIT("Failed to chdir to cwd", EXIT_COMMAND);
				
				if (chroot(".") == -1)
					PEXIT("Failed to chroot to cwd", EXIT_COMMAND);
			}
			// TODO: change TTY and stdin/out to something sensible for the guest!
			// -- check for /dev/console or /dev/tty in the guest's root
			if (opts.init) {
				vu_printf("Preparing for switching stdin to /dev/console...\n");
				struct stat st;
				if (stat("/dev/console", &st) == -1) {
					vu_printf("Failed to stat /dev/console:  %s\n", strerror(errno));
				} else if (S_ISCHR(st.st_mode)) {
					int fd = open("/dev/console", O_RDWR); // O_NOCTTY);
					if (fd < 0) vu_printf("Failed to open /dev/console:  %s\n", strerror(errno));
					if (fd > 0) dup2(fd, 0);
					if (fd >= 0 && fd != 1) dup2(fd, 1);
					if (fd >= 0 && fd != 2) dup2(fd, 2);
					if (fd > 2) close(fd);
				} else
					vu_printf("Cannot use /dev/console as it's not a character device.\n");
			}

			if(execvp(argv[optind], argv+optind) == -1)
				PEXIT("Failed to start init", EXIT_COMMAND);
		
		default:
			exit(EXIT_SUCCESS);
	}
}
