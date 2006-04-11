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
#include <signal.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "pathconfig.h"
#include "confuse.h"
#include "log.h"

static cfg_opt_t CFG_OPTS[] = {
	CFG_STR("listen-host",    "127.0.0.1", CFGF_NONE),
	CFG_INT("listen-port",    13386,       CFGF_NONE),
	CFG_INT("max-clients",    20,          CFGF_NONE),
	CFG_INT("client-timeout", 30,          CFGF_NONE),
	CFG_STR_LIST("admins",    NULL,        CFGF_NONE),
	CFG_END()
};

cfg_t *cfg;

void collector_main(void);
void server_main(void);

static inline
void usage(int rc)
{
	printf("Usage: vcd [<opts>*]\n\n"
	       "\n"
	       "Available options:\n"
	       "   -c <file>     configuration file (default: %s/vcd.conf)\n"
	       "   -d            debug mode (do not fork to background)\n",
	       __LOCALSTATEDIR);
	exit(rc);
}

int main(int argc, char **argv)
{
	char *cfg_file = __SYSCONFDIR "/vcd.conf";
	pid_t pid, server, collector;
	int c, status, debug = 0;
	
	/* getopt */
	while ((c = getopt (argc, argv, "dc:")) != -1) {
		switch (c) {
		case 'c':
			cfg_file = optarg;
			break;
		
		case 'd':
			debug = 1;
			break;
		
		default:
			usage(EXIT_FAILURE);
			break;
		}
	}
	
	if (argc > optind)
		usage(EXIT_FAILURE);
	
	/* load configuration */
	cfg = cfg_init(CFG_OPTS, CFGF_NOCASE);
	
	switch (cfg_parse(cfg, cfg_file)) {
	case CFG_FILE_ERROR:
		perror("cfg_parse");
		exit(EXIT_FAILURE);
	
	case CFG_PARSE_ERROR:
		printf("cfg_parse: parse error");
		exit(EXIT_FAILURE);
	
	default:
		break;
	}
	
	/* fork to background */
	if (debug)
		pid = 0;
	else
		pid = fork();
	
	switch (pid) {
	case -1:
		perror("fork()");
		exit(EXIT_FAILURE);
	
	case 0:
		break;
	
	default:
		exit(EXIT_SUCCESS);
	}
	
	/* open connection to syslog */
	openlog("vcd/master", LOG_CONS|LOG_PID|LOG_PERROR, LOG_DAEMON);
	
	/* daemon stuff */
	umask(0);
	setsid();
	
	if (chdir("/"))
		LOGPERR("chdir(/)");
	
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	
	if (!debug) {
		close(STDERR_FILENO);
		
		/* ignore some standard signals */
		signal(SIGHUP,  SIG_IGN);
		signal(SIGINT,  SIG_IGN);
	}
	
	/* start collector thread */
	switch ((collector = fork())) {
	case -1:
		LOGPERR("fork(collector)");
	
	case 0:
		collector_main();
	
	default:
		break;
	}
	
	/* start server thread */
	switch ((server = fork())) {
	case -1:
		LOGPERR("fork(server)");
	
	case 0:
		server_main();
	
	default:
		break;
	}
	
	/* our children only die due to errors */
	pid = waitpid(0, &status, 0);
	
	if (pid == -1) {
		if (errno == ECHILD)
			LOGERR("death of all children. following.");
		else
			LOGPWARN("waitpid()");
	}
	
	else if (pid == collector)
		LOGWARN("collector died. following.");
	
	else if (pid == server)
		LOGWARN("server died. following.");
	
	else
		LOGWARN("unknown child died. following.");
	
	kill(0, SIGTERM);
	return EXIT_FAILURE;
}
