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
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <syslog.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>
#include <lucid/log.h>
#include <lucid/open.h>
#include <lucid/tcp.h>

#include "cfg.h"
#include <lucid/log.h>
#include "methods.h"
#include "vxdb.h"

static cfg_opt_t CFG_OPTS[] = {
	CFG_STR_CB("host",    "127.0.0.1", CFGF_NONE, &cfg_validate_host),
	CFG_INT_CB("port",    13386,       CFGF_NONE, &cfg_validate_port),
	CFG_INT_CB("timeout", 30,          CFGF_NONE, &cfg_validate_timeout),
	
	CFG_STR("logfile", NULL, CFGF_NONE),
	CFG_STR("pidfile", NULL, CFGF_NONE),
	
	CFG_STR_CB("datadir",    LOCALSTATEDIR "/vcd", CFGF_NONE, &cfg_validate_path),
	CFG_STR_CB("vserverdir", VSERVERDIR,           CFGF_NONE, &cfg_validate_vdir),
	CFG_END()
};

cfg_t *cfg;

static inline
void usage(int rc)
{
	printf("Usage: vcd [<opts>*]\n"
	       "\n"
	       "Available options:\n"
	       "   -c <file>     configuration file (default: %s/vcd.conf)\n"
	       "   -d            debug mode (do not fork to background)\n",
	       SYSCONFDIR);
	exit(rc);
}

int main(int argc, char **argv)
{
	char *cfg_file = SYSCONFDIR "/vcd.conf";
	
	char c, *pidfile, *host;
	const char *logfile;
	int debug = 0, fd, port, timeout;
	pid_t pid;
	
	xmlrpc_env env;
	xmlrpc_server_abyss_parms serverparm;
	
	/* parse command line */
	while ((c = getopt(argc, argv, "dc:")) != -1) {
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
		dprintf(STDERR_FILENO, "cfg_parse: Parse error\n");
		exit(EXIT_FAILURE);
	
	default:
		break;
	}
	
	atexit(cfg_atexit);
	
	/* start logging & debugging */
	log_options_t log_options = {
		.ident  = argv[0],
		.file   = false,
		.stderr = false,
		.syslog = true,
		.flags  = LOG_PID,
	};
	
	logfile = cfg_getstr(cfg, "logfile");
	
	if (logfile && strlen(logfile) > 0) {
		log_options.fd = open_append(logfile);
		log_options.file = true;
	}
	
	if (debug) {
		log_options.stderr = true;
		log_options.mask   = LOG_UPTO(LOG_DEBUG);
	}
	
	log_init(&log_options);
	
	/* fork to background */
	if (!debug) {
		pid = fork();
		
		switch (pid) {
		case -1:
			log_perror_and_die("fork");
		
		case 0:
			break;
		
		default:
			exit(EXIT_SUCCESS);
		}
		
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}
	
	/* daemonize */
	close(STDIN_FILENO);
	umask(0);
	setsid();
	chdir("/");
	
	/* log process id */
	pidfile = cfg_getstr(cfg, "pidfile");
	
	if (pidfile && (fd = open_trunc(pidfile)) != -1) {
		dprintf(fd, "%d\n", getpid());
		close(fd);
	}
	
	/* open connection to vxdb */
	vxdb_init();
	atexit(vxdb_atexit);
	
	/* setup listen socket */
	port = cfg_getint(cfg, "port");
	host = cfg_getstr(cfg, "host");
	
	if ((fd = tcp_listen(host, port, 20)) == -1)
		log_perror_and_die("tcp_listen");
	
	/* setup xmlrpc server */
	xmlrpc_env_init(&env);
	
	registry = xmlrpc_registry_new(&env);
	method_registry_init(&env);
	atexit(method_registry_atexit);
	
	timeout = cfg_getint(cfg, "timeout");
	
	serverparm.config_file_name   = NULL;
	serverparm.registryP          = registry;
	serverparm.log_file_name      = NULL;
	serverparm.keepalive_timeout  = 0;
	serverparm.keepalive_max_conn = 0;
	serverparm.timeout            = timeout;
	serverparm.dont_advertise     = 0;
	serverparm.socket_bound       = 1;
	serverparm.socket_handle      = fd;
	
	log_info("Accepting incomming connections on %s:%d", host, port);
	
	xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(socket_handle));
	
	/* never get here */
	log_error_and_die("Unexpected death of Abyss server");
}
