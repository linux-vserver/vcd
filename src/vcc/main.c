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

#include "msg.h"

static char *uri  = NULL;
static char *user = "admin";
static char *pass = NULL;
static char *name = NULL;

static inline
void usage(int rc)
{
	printf("Usage: vcc <opts>* <command>\n"
	       "\n"
	       "Available commands:\n"
	       "   create <name> <template> [<rebuild>]\n"
	       "   remove <name>\n"
	       "   rename <name> <newname>\n"
	       "   start  <name>\n"
	       "   status <name>\n"
	       "   stop   <name>\n"
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

#define return_if_fault(ENV) do { \
	if (ENV->fault_occurred) return; \
} while(0)

#define SIGNATURE(S) "({s:s,s:s}" S ")", "username", user, "password", pass

static
void cmd_create(xmlrpc_env *env, int argc, char **argv)
{
	char *template;
	int rebuild = 0;
	
	if (argc != 1 && argc != 2)
		usage(EXIT_FAILURE);
	
	template = argv[0];
	
	if (argc == 2)
		rebuild = atoi(argv[1]);
	
	read_password();
	
	xmlrpc_client_call(env, uri, "vx.create",
		SIGNATURE("{s:s,s:s,s:i}"),
		"name", name,
		"template", template,
		"rebuild", rebuild);
}

static
void cmd_remove(xmlrpc_env *env, int argc, char **argv)
{
	read_password();
	
	xmlrpc_client_call(env, uri, "vx.remove",
		SIGNATURE("{s:s}"),
		"name", name);
}

static
void cmd_rename(xmlrpc_env *env, int argc, char **argv)
{
	char *newname;
	
	if (argc != 1)
		exit(EXIT_FAILURE);
	
	newname = argv[0];
	
	read_password();
	
	xmlrpc_client_call(env, uri, "vx.rename",
		SIGNATURE("{s:s}"),
		"name", name,
		"newname", newname);
}

static
void cmd_start(xmlrpc_env *env, int argc, char **argv)
{
	read_password();
	
	xmlrpc_client_call(env, uri, "vx.start",
		SIGNATURE("{s:s}"),
		"name", name);
}

static
void cmd_status(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *result;
	int running;
	
	read_password();
	
	result = xmlrpc_client_call(env, uri, "vx.status",
		SIGNATURE("{s:s}"),
		"name", name);
	return_if_fault(env);
	
	xmlrpc_decompose_value(env, result,
		"{s:i,*}",
		"running", &running);
	return_if_fault(env);
	
	xmlrpc_DECREF(result);
	
	printf("running: %d\n", running == 0 ? 0 : 1);
}

static
void cmd_stop(xmlrpc_env *env, int argc, char **argv)
{
	int wait, reboot;
	
	if (argc != 2)
		usage(EXIT_FAILURE);
	
	wait   = atoi(argv[0]);
	reboot = atoi(argv[1]);
	
	read_password();
	
	xmlrpc_client_call(env, uri, "vx.stop",
		SIGNATURE("{s:s,s:i,s:i}"),
		"name", name,
		"wait", wait,
		"reboot", reboot);
}

int main(int argc, char **argv)
{
	INIT_ARGV0
	
	char c, *cmd;
	char *host = "localhost";
	int port = 13386;
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
	
	xmlrpc_env_init(&env);
	
	xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, argv[0], PACKAGE_VERSION, NULL, 0);
	
	if (env.fault_occurred)
		err("failed to start xmlrpc client: %s", env.fault_string);
	
	asprintf(&uri, "http://%s:%d/RPC2", host, port);
	
	if (strcmp(cmd, "create") == 0)
		cmd_create(&env, argc - optind, argv + optind);
	
	else if (strcmp(cmd, "remove") == 0)
		cmd_remove(&env, argc - optind, argv + optind);
	
	else if (strcmp(cmd, "rename") == 0)
		cmd_rename(&env, argc - optind, argv + optind);
	
	else if (strcmp(cmd, "start") == 0)
		cmd_start(&env, argc - optind, argv + optind);
	
	else if (strcmp(cmd, "status") == 0)
		cmd_status(&env, argc - optind, argv + optind);
	
	else if (strcmp(cmd, "stop") == 0)
		cmd_stop(&env, argc - optind, argv + optind);
	
	else
		err("invalid command: '%s'\n", cmd);
	
	free(pass);
	free(uri);
	
	if (env.fault_occurred)
		err("%s: %s (%d)", cmd, env.fault_string, env.fault_code);
	
	xmlrpc_env_clean(&env);
	xmlrpc_client_cleanup();
	
	return EXIT_SUCCESS;
}
