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
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>

#include "cfg.h"
#include "methods.h"
#include "vxdb.h"

#include <lucid/log.h>
#include <lucid/mem.h>
#include <lucid/open.h>
#include <lucid/printf.h>
#include <lucid/str.h>
#include <lucid/tcp.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>

static cfg_opt_t CFG_OPTS[] = {
	CFG_STR_CB("host",    "127.0.0.1", CFGF_NONE, &cfg_validate_host),
	CFG_INT_CB("port",    13386,       CFGF_NONE, &cfg_validate_port),
	CFG_INT_CB("timeout", 30,          CFGF_NONE, &cfg_validate_timeout),

	CFG_STR("logfile", NULL, CFGF_NONE),
	CFG_STR("pidfile", NULL, CFGF_NONE),

	CFG_STR_CB("datadir",     LOCALSTATEDIR "/vcd",   CFGF_NONE,
			&cfg_validate_path),
	CFG_STR_CB("templatedir", VBASEDIR "/templates/", CFGF_NONE,
			&cfg_validate_path),
	CFG_STR_CB("vbasedir",    VBASEDIR,               CFGF_NONE,
			&cfg_validate_path),

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

static
void sigsegv_handler(int sig, siginfo_t *info, void *ucontext)
{
	log_error("Received SIGSEGV for virtual address %p (%d,%d)",
			info->si_addr, info->si_errno, info->si_code);

	log_error("You probably found a bug in vcd!");
	log_error("Please report it to hollow@gentoo.org");

	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	char *cfg_file = SYSCONFDIR "/vcd.conf";
	int c, debug = 0;

	/* install SIGSEGV handler */
	struct sigaction sa;

	sa.sa_sigaction = sigsegv_handler;
	sa.sa_flags = SA_RESETHAND|SA_SIGINFO;

	sigfillset(&sa.sa_mask);

	if (sigaction(SIGSEGV, &sa, NULL) == -1) {
		dprintf(STDERR_FILENO, "sigaction: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

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
		dprintf(STDERR_FILENO, "cfg_parse: %s\n", strerror(errno));
		exit(EXIT_FAILURE);

	case CFG_PARSE_ERROR:
		dprintf(STDERR_FILENO, "cfg_parse: Parse error\n");
		exit(EXIT_FAILURE);

	default:
		break;
	}

	/* free configuration on exit */
	atexit(cfg_atexit);

	/* free all memory on exit */
	atexit(mem_freeall);

	/* start logging & debugging */
	log_options_t log_options = {
		.ident  = argv[0],
		.file   = false,
		.stderr = false,
		.syslog = true,
		.time   = true,
		.flags  = LOG_PID,
	};

	const char *logfile = cfg_getstr(cfg, "logfile");

	if (logfile && str_len(logfile) > 0) {
		log_options.file = true;
		log_options.fd   = open_append(logfile);
	}

	if (debug) {
		log_options.stderr = true;
		log_options.mask   = LOG_UPTO(LOG_DEBUG);
	}

	log_init(&log_options);

	/* close log multiplexer on exit */
	atexit(log_close);

	/* fork to background */
	if (!debug) {
		log_info("Running in background mode ...");

		pid_t pid = fork();

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

	else
		log_info("Running in debugging mode ...");

	/* daemonize */
	close(STDIN_FILENO);
	umask(0);
	setsid();
	chdir("/");

	/* log process id */
	int fd;
	const char *pidfile = cfg_getstr(cfg, "pidfile");

	if (pidfile && (fd = open_trunc(pidfile)) != -1) {
		dprintf(fd, "%d\n", getpid());
		close(fd);
	}

	/* open connection to vxdb */
	vxdb_init();
	atexit(vxdb_atexit);

	/* setup listen socket */
	int port = cfg_getint(cfg, "port");
	const char *host = cfg_getstr(cfg, "host");

	if ((fd = tcp_listen(host, port, 20)) == -1)
		log_perror_and_die("tcp_listen(%s,%hu)", host, port);

	/* setup xmlrpc server */
	xmlrpc_env env;
	xmlrpc_env_init(&env);

	registry = xmlrpc_registry_new(&env);
	method_registry_init(&env);
	atexit(method_registry_atexit);

	int timeout = cfg_getint(cfg, "timeout");

	xmlrpc_server_abyss_parms serverparm = {
		.config_file_name   = NULL,
		.registryP          = registry,
		.log_file_name      = NULL,
		.keepalive_timeout  = 0,
		.keepalive_max_conn = 0,
		.timeout            = timeout,
		.dont_advertise     = 0, /* TODO: what's this? */
		.socket_bound       = 1,
		.socket_handle      = fd,
	};

	log_info("Accepting incomming connections on %s:%d", host, port);

	xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(socket_handle));

	/* never get here */
	log_error_and_die("Unexpected death of Abyss server");
}
