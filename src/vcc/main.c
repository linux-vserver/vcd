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
#include <getopt.h>
#include <termios.h>
#include <confuse.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include <lucid/log.h>
#include <lucid/misc.h>
#include <lucid/printf.h>
#include <lucid/scanf.h>
#include <lucid/str.h>

#include "cmd.h"

static char *host = "localhost";
static int   port = 13386;

static cfg_opt_t CFG_OPTS[] = {
	CFG_STR("host", "localhost", CFGF_NONE),
	CFG_INT("port", 13386,       CFGF_NONE),
	CFG_STR("user", "admin",     CFGF_NONE),
	CFG_STR("pass", NULL,        CFGF_NONE),
	CFG_END()
};

void usage(int rc)
{
	printf("Usage: vcc <opts>* <command>\n"
	       "\n"
	       "Available commands:\n"
	       "   create  <name> <template> [<force> [copy [<vdir>]]]\n"
	       "   exec    <name> <program> <args>*\n"
	       "   kill    <name>\n"
	       "   list    [<username>]\n"
	       "   load    <name>\n"
	       "   login   <name>\n"
	       "   reboot  <name>\n"
	       "   remove  <name>\n"
	       "   rename  <name> <newname>\n"
	       "   start   <name>\n"
	       "   status  <name>\n"
	       "   stop    <name>\n"
	       "   templates [<name>]\n"
	       "\n"
	       "Available options:\n"
	       "   -c <path>     configuration file (default: %s/vcc.conf)\n"
	       "   -h <host>     server hostname (default: localhost)\n"
	       "   -p <port>     server port     (default: 13386)\n"
	       "   -u <user>     server username (default: admin)\n",
	       SYSCONFDIR);
	exit(rc);
}

static
void read_config(char *file, int ignore_noent)
{
	cfg_t *cfg;
	
	cfg = cfg_init(CFG_OPTS, CFGF_NOCASE);

	if (ignore_noent && !isfile(file))
		file = "/dev/null";
	
	switch (cfg_parse(cfg, file)) {
	case CFG_FILE_ERROR:
		log_perror_and_die("cfg_parse");
	
	case CFG_PARSE_ERROR:
		log_error_and_die("cfg_parse: Parse error\n");
	
	default:
		break;
	}
	
	host = cfg_getstr(cfg, "host");
	port = cfg_getint(cfg, "port");
	user = cfg_getstr(cfg, "user");
	pass = cfg_getstr(cfg, "pass");
}

static
void read_password(void)
{
	struct termios tty, oldtty;
	
	if (pass)
		return;
	
	write(STDOUT_FILENO, "password: ", 10);
	
	/* save original terminal settings */
	if (tcgetattr(STDIN_FILENO, &oldtty) == -1)
		log_perror_and_die("tcgetattr");
	
	tty = oldtty;
	
	tty.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
	tty.c_cc[VMIN]  = 1;
	tty.c_cc[VTIME] = 0;
	
	/* apply new terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty) == -1)
		log_perror_and_die("tcsetattr");
	
	str_readline(STDIN_FILENO, &pass);
	
	/* apply old terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldtty) == -1)
		log_perror_and_die("tcsetattr");
	
	write(STDOUT_FILENO, "\n", 1);
}

int main(int argc, char **argv)
{
	char *cmd;
	xmlrpc_env env;
	
	log_options_t log_options = {
		.log_ident = argv[0],
		.log_dest  = LOGD_STDERR,
		.log_opts  = LOGO_PRIO|LOGO_IDENT,
	};
	
	log_init(&log_options);

	read_config(SYSCONFDIR "/vcc.conf", 1);
	
	char c;
	
	/* parse command line */
	while ((c = getopt(argc, argv, "c:h:p:u:")) != -1) {
		switch (c) {
		case 'c':
			read_config(optarg, 0);
			break;
		
		case 'h':
			host = optarg;
			break;
		
		case 'p':
			sscanf(optarg, "%d", &port);
			break;
		
		case 'u':
			user = optarg;
			break;
		
		default:
			usage(EXIT_FAILURE);
			break;
		}
	}
	
	if (argc < optind + 1)
		usage(EXIT_FAILURE);
	
	cmd  = argv[optind++];

	if (str_equal(cmd, "list") || str_equal(cmd, "templates"))
		name = NULL;
	else if (argc < optind + 2)
		usage(EXIT_FAILURE);
	else
		name = argv[optind++];

	int i;

	for (i = 0; CMDS[i].name; i++)
		if (str_equal(cmd, CMDS[i].name))
			goto init;
	
	log_error_and_die("invalid command: %s", cmd);
	
init:
	xmlrpc_env_init(&env);
	
	xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, 
			argv[0], PACKAGE_VERSION, NULL, 0);
	
	if (env.fault_occurred)
		log_error_and_die("failed to start xmlrpc client: %s", env.fault_string);
	
	asprintf(&uri, "http://%s:%d/RPC2", host, port);
	
	read_password();
	
	for (i = 0; CMDS[i].name; i++)
		if (str_equal(cmd, CMDS[i].name))
			CMDS[i].func(&env, argc - optind, argv + optind);
	
	if (env.fault_occurred)
		log_error_and_die("%s: %s (%d)", cmd, env.fault_string, env.fault_code);
	
	xmlrpc_env_clean(&env);
	xmlrpc_client_cleanup();
	
	return EXIT_SUCCESS;
}
