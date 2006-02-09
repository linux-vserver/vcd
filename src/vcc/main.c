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
#include <strings.h>
#include <sys/poll.h>

#include "vc.h"
#include "commands.h"

static const char *rcsid = "$Id$";

int interactive = 0;

vcc_command_t commands[] = {
	{ "start", start_main },
	{ NULL,    NULL },
};

static
int usage(int rc)
{
	int i;
	
	if (interactive == 1) {
		vc_printf("usage: <command> [args]\n"
		          "Type '<command> help' for help on a specific command.\n"
		          "\n"
		          "Available commands:\n");
	} else {
		vc_printf("usage: vcc <command> [args]\n"
		          "Type 'vcc <command> --help' for help on a specific command.\n"
		          "\n"
		          "Available commands:\n");
	}
	
	for (i = 0; commands[i].name; i++)
		vc_printf("  %s\n", commands[i].name);
	
	vc_printf("\n");
	
	return rc;
}

static
int do_command(int argc, char **argv)
{
	char *name = argv[0];
	int i;
	
	/* catch help */
	if (strcmp(name, "help") == 0 && interactive)
		return usage(EXIT_SUCCESS);
	
	if (strcmp(name, "--help") == 0 && !interactive)
		return usage(EXIT_SUCCESS);
	
	/* search command list */
	for (i = 0; commands[i].name; i++) {
		if (strcasecmp(commands[i].name, name) == 0)
			return commands[i].func(argc, argv);
	}
	
	vc_warn("%s: Function not supported", name);
	
	return EXIT_FAILURE;
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
	int rc = 0, num;
	int len;
	
	char *line = NULL;
	int ac;
	char **av = NULL;
	
	/* interative mode */
	if (argc < 2) {
		interactive = 1;
		
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
			else if (len == 0)
				rc = 0;
			
			/* run command */
			else if (len > 0) {
				av = vc_str_to_argv(line, &ac);
				
				/* catch exit commands */
				if (strcmp(av[0], "logout") == 0 ||
				    strcmp(av[0], "exit")   == 0 ||
				    strcmp(av[0], "quit")   == 0)
					break;
				
				else
					rc = do_command(ac, av);
			}
		}
	}
	
	/* call command directly and return */
	else
		return do_command(argc-1, argv+1);
	
	return EXIT_SUCCESS;
}
