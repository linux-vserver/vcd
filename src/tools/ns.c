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

#include <stdlib.h>
#include <vserver.h>
#include <wait.h>

#include "vc.h"
#include "tools.h"

/* dietlibc does not define CLONE_NEWNS */
#ifndef CLONE_NEWNS
#define CLONE_NEWNS 0x00020000
#endif

static const char *rcsid = "$Id$";

static
struct option long_opts[] = {
	COMMON_LONG_OPTS
	{ "create",    1, 0, 0x10 },
	{ "migrate",   1, 0, 0x11 },
	{ NULL,        0, 0, 0 },
};

static inline
void usage(int rc)
{
	vc_printf("Usage:\n\n"
	          "ns -create  <xid>\n"
	          "   -migrate <xid> -- <program> <args>*\n");
	exit(rc);
}

int main(int argc, char *argv[])
{
	VC_INIT_ARGV0
	
	int c, status;
	xid_t xid = 0;
	pid_t pid;
	
#define CASE_GOTO(ID, P) case ID: xid = atoi(optarg); goto P; break
	
	/* parse command line */
	while (GETOPT(c)) {
		switch (c) {
			COMMON_GETOPT_CASES
			
			CASE_GOTO(0x10, create);
			CASE_GOTO(0x11, migrate);
			
			DEFAULT_GETOPT_CASES
		}
	}
	
#undef CASE_GOTO
	
	goto usage;
	
create:
	signal(SIGCHLD, SIG_DFL);
	
	pid = sys_clone(CLONE_NEWNS|SIGCHLD, 0);
	
	switch(pid) {
		case -1:
			vc_errp("sys_clone");
		
		case 0:
			if (vx_set_namespace(xid) == -1)
				vc_errp("vx_set_namespace");
			
			goto out;
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				vc_errp("waitpid");
		
			if (WIFEXITED(status))
				exit(WEXITSTATUS(status));
			
			if (WIFSIGNALED(status)) {
				vc_printf("Child interrupted by signal; following...\n");
				kill(getpid(), WTERMSIG(status));
				exit(1);
			}
	}
	
	goto out;
	
migrate:
	if (vx_enter_namespace(xid) == -1)
		vc_errp("vx_enter_namespace");
	
	if (argc > optind+1)
		execvp(argv[optind+1], argv+optind+1);
	
	goto out;
	
usage:
	usage(EXIT_FAILURE);

out:
	exit(EXIT_SUCCESS);
}
