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
#include <confuse.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>
#include <lucid/io.h>
#include <lucid/log.h>

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
	printf("Usage: vce <opts>* <command>\n"
	       "\n"
	       "Available commands:\n"
	       "   dx.limit.get        <name> [<path>]\n"
	       "   dx.limit.remove     <name> [<path>]\n"
	       "   dx.limit.set        <name> <path> <space> <inodes> <reserved>\n"
	       "\n"
	       "   init.get            <name>\n"
	       "   init.set            <name> <init> <halt> <reboot>\n"
	       "\n"
	       "   mount.get           <name> [<dst>]\n"
	       "   mount.remove        <name> [<dst>]\n"
	       "   mount.set           <name> <src> <dst> <type> <opts>\n"
	       "\n"
	       "   nx.broadcast.get    <name>\n"
	       "   nx.broadcast.remove <name>\n"
	       "   nx.broadcast.set    <name> <broadcast>\n"
	       "\n"
	       "   nx.addr.get         <name> [<addr>]\n"
	       "   nx.addr.remove      <name> [<addr>]\n"
	       "   nx.addr.set         <name> <addr> [<netmask>]\n"
	       "\n"
	       "   vx.bcaps.add        <name> <bcap>\n"
	       "   vx.bcaps.get        <name>\n"
	       "   vx.bcaps.remove     <name> [<bcap>]\n"
	       "\n"
	       "   vx.ccaps.add        <name> <ccap>\n"
	       "   vx.ccaps.get        <name>\n"
	       "   vx.ccaps.remove     <name> [<ccap>]\n"
	       "\n"
	       "   vx.flags.add        <name> <flag>\n"
	       "   vx.flags.get        <name>\n"
	       "   vx.flags.remove     <name> [<flag>]\n"
	       "\n"
	       "   vx.limit.get        <name> [<limit>]\n"
	       "   vx.limit.remove     <name> [<limit>]\n"
	       "   vx.limit.set        <name> <limit> <soft> [<max>]\n"
	       "\n"
	       "   vx.sched.get        <name> [<cpuid>]\n"
	       "   vx.sched.remove     <name> [<cpuid>]\n"
	       "   vx.sched.set        <name> [<cpuid>] <bucket>\n"
	       "\n"
	       "   <bucket> = <int> <fill> <int2> <fill2> <min> <max>\n"
	       "\n"
	       "   vx.uname.get        <name> [<uname>]\n"
	       "   vx.uname.remove     <name> [<uname>]\n"
	       "   vx.uname.set        <name> <uname> <value>\n"
	       "\n"
	       "Available options:\n"
	       "   -c <path>     configuration file (default: %s/vce.conf)\n"
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
	
	switch (cfg_parse(cfg, file)) {
	case CFG_FILE_ERROR:
		if (ignore_noent)
			break;
		else
			log_perror_and_die("cfg_parse");
	
	case CFG_PARSE_ERROR:
		dprintf(STDERR_FILENO, "cfg_parse: Parse error\n");
		exit(EXIT_FAILURE);
	
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
	
	io_read_eol(STDIN_FILENO, &pass);
	
	/* apply old terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldtty) == -1)
		log_perror_and_die("tcsetattr");
	
	write(STDOUT_FILENO, "\n", 1);
}

int main(int argc, char **argv)
{
	char c, *cmd;
	int i;
	xmlrpc_env env;
	
	log_options_t log_options = {
		.ident  = argv[0],
		.file   = false,
		.stderr = true,
		.syslog = false,
	};
	
	log_init(&log_options);
	
	read_config(SYSCONFDIR "/vce.conf", 1);
	
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
	
	log_error_and_die("invalid command: %s", cmd);
	
init:
	xmlrpc_env_init(&env);
	
	xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, argv[0], PACKAGE_VERSION, NULL, 0);
	
	if (env.fault_occurred)
		log_error_and_die("failed to start xmlrpc client: %s", env.fault_string);
	
	asprintf(&uri, "http://%s:%d/RPC2", host, port);
	
	read_password();
	
	for (i = 0; CMDS[i].name; i++)
		if (strcmp(cmd, CMDS[i].name) == 0)
			CMDS[i].func(&env, argc - optind, argv + optind);
	
	free(pass);
	free(uri);
	
	if (env.fault_occurred)
		log_error_and_die("%s: %s (%d)", cmd, env.fault_string, env.fault_code);
	
	xmlrpc_env_clean(&env);
	xmlrpc_client_cleanup();
	
	return EXIT_SUCCESS;
}
