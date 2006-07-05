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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <termios.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "lucid.h"

#include "cmd.h"
#include "msg.h"

void usage(int rc)
{
	printf("Usage: vcc <opts>* <command>\n"
	       "\n"
	       "Available commands:\n"
	       "   create  <name> <template> [<rebuild>]\n"
	       "   exec    <name> <program> <args>*\n"
	       "   login   <name>\n"
	       "   remove  <name>\n"
	       "   rename  <name> <newname>\n"
	       "   restart <name>\n"
	       "   start   <name>\n"
	       "   status  <name>\n"
	       "   stop    <name>\n"
	       "\n"
	       "Available options:\n"
	       "   -h <host>     server hostname (default: localhost)\n"
	       "   -p <port>     server port     (default: 13386)\n"
	       "   -u <user>     server username (default: admin)\n");
	exit(rc);
}

static
void read_password(void)
{
	struct termios tty, oldtty;
	
	write(STDOUT_FILENO, "password: ", 10);
	
	/* save original terminal settings */
	if (tcgetattr(STDIN_FILENO, &oldtty) == -1)
		perr("tcgetattr");
	
	tty = oldtty;
	
	tty.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
	tty.c_cc[VMIN]  = 1;
	tty.c_cc[VTIME] = 0;
	
	/* apply new terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty) == -1)
		perr("tcsetattr");
	
	io_read_eol(STDIN_FILENO, &pass);
	
	/* apply old terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldtty) == -1)
		perr("tcsetattr");
	
	write(STDOUT_FILENO, "\n", 1);
}

int main(int argc, char **argv)
{
	INIT_ARGV0
	
	char c, *cmd;
	char *host = "localhost";
	int i, port = 13386;
	xmlrpc_env env;
	
	/* parse command line */
	while ((c = getopt(argc, argv, "h:p:u:")) != -1) {
		switch (c) {
		case 'h':
			host = optarg;
			break;
		
		case 'p':
			port = atoi(optarg);
			break;
		
		case 'u':
			user = optarg;
			break;
		
		default:
			usage(EXIT_FAILURE);
			break;
		}
	}
	
	if (argc < optind + 2)
		usage(EXIT_FAILURE);
	
	cmd  = argv[optind++];
	name = argv[optind++];
	
	for (i = 0; CMDS[i].name; i++)
		if (strcmp(cmd, CMDS[i].name) == 0)
			goto init;
	
	err("invalid command: %s", cmd);
	
init:
	xmlrpc_env_init(&env);
	
	xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, argv[0], PACKAGE_VERSION, NULL, 0);
	
	if (env.fault_occurred)
		err("failed to start xmlrpc client: %s", env.fault_string);
	
	asprintf(&uri, "http://%s:%d/RPC2", host, port);
	
	read_password();
	
	for (i = 0; CMDS[i].name; i++)
		if (strcmp(cmd, CMDS[i].name) == 0)
			CMDS[i].func(&env, argc - optind, argv + optind);
	
	free(pass);
	free(uri);
	
	if (env.fault_occurred)
		err("%s: %s (%d)", cmd, env.fault_string, env.fault_code);
	
	xmlrpc_env_clean(&env);
	xmlrpc_client_cleanup();
	
	return EXIT_SUCCESS;
}
