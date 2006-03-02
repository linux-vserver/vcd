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

#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <wait.h>
#include <syslog.h>
#include <lucid/io.h>

#include "commands.h"

/* command map */
vc_cmd_t CMDS[] = {
	{ "login", login_main, login_help, 1 },
	{ "start", start_main, start_help, 0 },
	{ NULL,    NULL,       NULL,       0 },
};

int do_command(int argc, char **argv, char **data)
{
	int i, status;
	pid_t pid;
	
	if(data == NULL) {
		switch((pid = fork())) {
			case -1:
				return -1;
			
			case 0:
				for (i = 0; CMDS[i].name; i++) {
					if (strcasecmp(CMDS[i].name, argv[0]) == 0)
						CMDS[i].main(argc, argv);
				}
				
				exit(EXIT_FAILURE);
			
			default:
				if (waitpid(pid, &status, 0) == -1)
					return -1;
				else if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
					return EXIT_SUCCESS;
				else
					return EXIT_FAILURE;
		}
	}
	
	else {
		int pfd[2];
		
		if (pipe(pfd) == -1)
			return -1;
		
		switch((pid = fork())) {
			case -1:
				return -1;
			
			case 0:
				close(pfd[0]);
				close(STDIN_FILENO);
				dup2(pfd[1], STDOUT_FILENO);
				dup2(pfd[1], STDERR_FILENO);
				
				for (i = 0; CMDS[i].name; i++) {
					if (strcasecmp(CMDS[i].name, argv[0]) == 0 && CMDS[i].interactive == 0)
						CMDS[i].main(argc, argv);
				}
				
				exit(EXIT_FAILURE);
			
			default:
				close(pfd[1]);
				
				if (io_read_eof(pfd[0], data) == -1) {
					close(pfd[0]);
					return -1;
				}
				
				close(pfd[0]);
				
				if (waitpid(pid, &status, 0) == -1)
					return -1;
				else if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
					return EXIT_SUCCESS;
				else
					return EXIT_FAILURE;
		}
	}
	
	/* we should never get here */
	return -1;
}

