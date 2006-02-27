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

#define _GNU_SOURCE
#include <string.h>

#include <stdlib.h>
#include <strings.h>
#include <wait.h>
#include <sys/poll.h>
#include <lucid/argv.h>

#include "vc.h"
#include "commands.h"

static const char *rcsid = "$Id$";

int vcc_interactive = 0;

vcc_command_t commands[] = {
	{ "login", login_main, login_usage },
	{ "start", start_main, start_usage },
	{ NULL,    NULL,       NULL },
};

static
int usage(int rc, char *command)
{
	int i;
	
	/* display command help */
	if (command != NULL) {
		for (i = 0; commands[i].name; i++) {
			if (strcasecmp(commands[i].name, command) == 0)
				commands[i].help(rc);
		}
		
		vc_err("%s: Function not supported", command);
	}
	
	/* display global help */
	else {
		if (vcc_interactive == 1) {
			vc_printf("usage: <command> [args]\n"
								"Type 'help <command>' for help on a specific command.\n"
								"\n"
								"Available commands:\n");
		} else {
			vc_printf("usage: vcc <command> [args]\n"
								"Type 'vcc --help <command>' for help on a specific command.\n"
								"\n"
								"Available commands:\n");
		}
		
		for (i = 0; commands[i].name; i++)
			vc_printf("  %s\n", commands[i].name);
		
		vc_printf("\n%s\n", rcsid);
	}
	
	exit(rc);
}

static
void do_command(int argc, char **argv)
{
	char *name = argv[0];
	int i;
	
	/* catch help */
	if (strcmp(name, "help") == 0 && vcc_interactive) {
		if (argc > 1)
			usage(EXIT_SUCCESS, argv[1]);
		else
			usage(EXIT_SUCCESS, NULL);
	}
	
	if (strcmp(name, "--help") == 0 && !vcc_interactive) {
		if (argc > 1)
			usage(EXIT_SUCCESS, argv[1]);
		else
			usage(EXIT_SUCCESS, NULL);
	}
	
	/* search command list */
	for (i = 0; commands[i].name; i++) {
		if (strcasecmp(commands[i].name, name) == 0)
			commands[i].func(argc, argv);
	}
	
	vc_warn("%s: Function not supported", name);
	exit(EXIT_FAILURE);
}

#define CHUNKSIZE 32

/* TODO: move this to lucid */
int vc_readline(int fd, char **line)
{
	int chunks = 1, idx = 0;
	char *buf = malloc(chunks * CHUNKSIZE + 1);
	char c;

	for (;;) {
		switch(read(fd, &c, 1)) {
			case -1:
				return -1;
			
			case 0:
				return -2;
			
			default:
				if (c == '\n')
					goto out;
				
				if (idx >= chunks * CHUNKSIZE) {
					chunks++;
					buf = realloc(buf, chunks * CHUNKSIZE + 1);
				}
				
				buf[idx++] = c;
				break;
		}
	}
	
out:
	buf[idx] = '\0';
	*line = buf;
	return strlen(buf);
}

int main(int argc, char *argv[])
{
	VC_INIT_ARGV0
	
	struct pollfd ufds = {
		.fd      = STDIN_FILENO,
		.events  = POLLIN,
		.revents = 0,
	};
	
	/* for interactive mode */
	pid_t pid;
	int status, rc = 0, num;
	int len;
	
	char *line = NULL;
	int ac;
	char **av = NULL;
	
	/* interative mode */
	if (argc < 2) {
		vcc_interactive = 1;
		
		for (num = 0;; num++) {
			vc_printf("vcc[%d:%d]> ", num, rc);
			
			len = vc_readline(STDIN_FILENO, &line);
			
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
				rc = 0;
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
				
				/* TODO: vc_argv_free */
				else {
					switch((pid = fork())) {
						case -1:
							vc_warnp("fork");
							rc = EXIT_FAILURE;
							break;
						
						case 0:
							do_command(ac, av);
							break;
						
						default:
							if (waitpid(pid, &status, 0) == -1) {
								vc_warnp("waitpid");
								rc = EXIT_FAILURE;
							}
							
							if (WIFEXITED(status)) {
								if (WEXITSTATUS(status) == EXIT_SUCCESS)
									rc = EXIT_SUCCESS;
								else
									rc = EXIT_FAILURE;
							}
							
							if (WIFSIGNALED(status)) {
								vc_printf("--- %s ---\n", strsignal(WTERMSIG(status)));
								rc = EXIT_FAILURE;
							}
					}
				}
			}
		}
	}
	
	/* call command directly */
	else
		do_command(argc-1, argv+1);
	
	return EXIT_SUCCESS;
}
