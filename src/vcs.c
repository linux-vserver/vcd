// Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#define _GNU_SOURCE
#include <string.h>

#include <stdlib.h>
#include <strings.h>
#include <wait.h>
#include <sys/poll.h>
#include <lucid/argv.h>
#include <lucid/io.h>

#include "vc.h"
#include "commands.h"

static const char *rcsid = "$Id$";

int main(int argc, char *argv[])
{
	VC_INIT_ARGV0
	
	struct pollfd ufds = {
		.fd      = STDIN_FILENO,
		.events  = POLLIN,
		.revents = 0,
	};
	
	pid_t pid;
	int status = EXIT_SUCCESS;
	
	int len;
	char *line = NULL;
	
	int ac;
	char **av = NULL;
	
	/* interative mode */
	while (1) {
		vc_printf("vcs[%d]> ", status);
		
		len = io_read_eol(STDIN_FILENO, &line);
		
		if (len == -1)
			vc_errp("vc_readline");
		
		/* EOF */
		else if (len == -2) {
			vc_printf("exit\n");
			break;
		}
		
		/* no command given */
		else if (len == 0) {
			free(line);
			status = 0;
		}
		
		/* run command */
		else if (len > 0) {
			argv_parse(line, &ac, &av);
			free(line);
			
			/* catch exit commands */
			if (strcmp(av[0], "logout") == 0 ||
					strcmp(av[0], "exit")   == 0 ||
					strcmp(av[0], "quit")   == 0) {
				argv_free(ac, av);
				break;
			}
			
			else {
				switch((pid = fork())) {
					case -1:
						vc_warnp("fork");
						status = EXIT_FAILURE;
						break;
					
					case 0:
						do_command(ac, av, NULL);
						break;
					
					default:
						if (waitpid(pid, &status, 0) == -1) {
							vc_warnp("waitpid");
							status = EXIT_FAILURE;
						}
						
						if (WIFEXITED(status)) {
							if (WEXITSTATUS(status) == EXIT_SUCCESS)
								status = EXIT_SUCCESS;
							else
								status = EXIT_FAILURE;
						}
						
						else if (WIFSIGNALED(status)) {
							vc_printf("--- %s ---\n", strsignal(WTERMSIG(status)));
							status = EXIT_FAILURE;
						}
				}
				
				argv_free(ac, av);
			}
		}
	}
	
	return EXIT_SUCCESS;
}
