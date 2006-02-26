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
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vserver.h>

#include "printf.h"
#include "tools.h"

#define NAME  "vnamespace"
#define DESCR "Filesystem Namespace Manager"

#define SHORT_OPTS "CENSfx:"

/* dietlibc does not define CLONE_NEWNS */
#ifndef CLONE_NEWNS
#define CLONE_NEWNS 0x00020000
#endif

struct commands {
	bool cleanup;
	bool enter;
	bool new;
	bool set;
};

struct options {
	bool force_clean;
	xid_t xid;
};

static inline
void cmd_help()
{
	vu_printf("Usage: %s <command> <opts>* -- <program> <args>*\n"
	       "\n"
	       "Available commands:\n"
	       "    -C            Remove all mounts from current context\n"
	       "    -E            Enter the namespace of context <xid>\n"
	       "    -N            Create a new namespace\n"
	       "    -S            Make current namespace the namespace of current context\n"
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
	struct commands cmds = {
		.cleanup = false,
		.enter   = false,
		.new     = false,
		.set     = false,
	};
	
	struct options opts = {
		.force_clean = false,
		.xid         = 0,
	};
	
	int c;
	pid_t pid; /* sys_clone */
	int status; /* waitpid */
	char cwd[PATH_MAX]; /* chdir */
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'C':
				cmds.cleanup = true;
				break;
			
			case 'E':
				cmds.enter = true;
				break;
			
			case 'N':
				cmds.new = true;
				break;
			
			case 'S':
				cmds.set = true;
				break;
			
			case 'f':
				opts.force_clean = true;
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	if (cmds.cleanup) {
		if (!opts.force_clean)
			SEXIT("You don't want this. Really. (Use -f if you are sure)", EXIT_USAGE);
		
		if (vx_cleanup_namespace() == -1)
			PEXIT("Failed to cleanup namespace", EXIT_COMMAND);
		
		goto out;
	}
	
	if (cmds.enter) {
		if (getcwd(cwd, PATH_MAX) == NULL)
			PEXIT("Failed to get cwd", EXIT_COMMAND);
		
		if (vx_enter_namespace(opts.xid) == -1)
			PEXIT("Failed to enter namespace", EXIT_COMMAND);
		
		if (chdir(cwd) == -1)
			PEXIT("Failed to restore cwd", EXIT_COMMAND);
		
		goto load;
	}
	
	if (cmds.new) {
		signal(SIGCHLD, SIG_DFL);
		
		pid = sys_clone(CLONE_NEWNS|SIGCHLD, 0);
		
		switch(pid) {
			case -1:
				PEXIT("Failed to create new namespace", EXIT_COMMAND);
			
			case 0:
				if (vx_set_namespace(opts.xid) == -1)
					PEXIT("Failed to set namespace", EXIT_COMMAND);
				goto out;
			
			default:
				if (waitpid(pid, &status, 0) == -1)
					PEXIT("Failed to wait for child", EXIT_COMMAND);
			
				if (WIFEXITED(status))
					exit(WEXITSTATUS(status));
				
				if (WIFSIGNALED(status)) {
				 vu_printf("Child interrupted by signal; following...\n");
					kill(getpid(), WTERMSIG(status));
					exit(1);
				}
		}
	}
	
	if (cmds.set) {
		if (vx_set_namespace(opts.xid) == -1)
			PEXIT("Failed to set namespace", EXIT_COMMAND);
		
		goto out;
	}
	
	cmd_help();
	goto out;
	
load:
	if (argc > optind)
		execvp(argv[optind], argv+optind);
	
out:
	exit(EXIT_SUCCESS);
}
