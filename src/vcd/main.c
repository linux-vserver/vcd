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
#include <sys/wait.h>
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

static log_options_t log_options = {
	.log_ident    = NULL,
	.log_dest     = LOGD_SYSLOG,
	.log_opts     = LOGO_PRIO|LOGO_TIME|LOGO_IDENT|LOGO_PID,
	.log_facility = LOG_DAEMON,
};

static char *cfg_file = SYSCONFDIR "/vcd.conf";
static int sfd = -1;
static int masterpid = -1;
static xmlrpc_env *env = NULL;

static inline
void usage(int rc)
{
	printf("Usage: vcd [<opts>*]\n"
			"\n"
			"Available options:\n"
			"   -b            fork to background (daemonize)\n"
			"   -c <file>     configuration file (default: %s/vcd.conf)\n"
			"   -d            debug mode (do not fork to background)\n",
			SYSCONFDIR);
	exit(rc);
}

static inline
void init_stats(void)
{
	LOG_TRACEME

	int rc = vxdb_exec("INSERT OR REPLACE INTO vcd "
			"(uptime, requests, flogins, nomethod, vxdbqueries) "
			"VALUES (%d, 0, 0, 0, 0)", time(NULL));

	if (rc != VXDB_OK)
		log_error("cannot initialize internal statistics: %s",
				vxdb_errmsg(vxdb));
}

static inline
void init_listen_socket(void)
{
	LOG_TRACEME

	int port = cfg_getint(cfg, "port");
	const char *host = cfg_getstr(cfg, "host");

	if ((sfd = tcp_listen(host, port, 20)) == -1)
		log_perror_and_die("tcp_listen(%s,%hu)", host, port);

	log_info("accepting incomming connections on %s:%d", host, port);
}

static
void shutdown_listen_socket(void)
{
	LOG_TRACEME

	if (getpid() != masterpid)
		return;

	/* close listen socket, so no new connections arrive */
	close(sfd);

	/* now wait for pending connections */
	int status;

	log_info("waiting for pending connections");

	while (waitpid(-1, &status, 0) > 0);
}

static inline
void init_server(void)
{
	LOG_TRACEME

	registry = xmlrpc_registry_new(env);
	method_registry_init(env);

	atexit(method_registry_atexit);

	int timeout = cfg_getint(cfg, "timeout");

	xmlrpc_server_abyss_parms serverparm = {
		.config_file_name   = NULL,
		.registryP          = registry,
		.log_file_name      = NULL,
		.keepalive_timeout  = 0,
		.keepalive_max_conn = 0,
		.timeout            = timeout,
		.dont_advertise     = 1,
		.socket_bound       = 1,
		.socket_handle      = sfd,
	};

	xmlrpc_server_abyss(env, &serverparm, XMLRPC_APSIZE(socket_handle));

	log_error_and_die("unexpected death of server");
}

static inline
void reload(void)
{
	LOG_TRACEME

	/* reload config files first */
	cfg_atexit();
	cfg_load(cfg_file);

	/* now reopen log files */
	log_close();

	const char *logfile = cfg_getstr(cfg, "logfile");

	log_options.log_dest &= ~LOGD_FILE;

	if (!str_isempty(logfile)) {
		log_options.log_dest |= LOGD_FILE;
		log_options.log_fd = open_append(logfile);
	}

	log_init(&log_options);

	/* shutdown listen socket */
	shutdown_listen_socket();

	/* reinitialize internal stats */
	init_stats();

	/* reload vxdb */
	vxdb_atexit();
	vxdb_init();

	/* reopen listen socket */
	init_listen_socket();

	/* start xmlrpc server again */
	init_server();
}

static
void signal_handler(int sig, siginfo_t *info, void *ucontext)
{
	switch (sig) {
	case SIGHUP:
		/* FIXME: do we really want to exit here? */
		if (getpid() != masterpid) {
			log_info("caught SIGHUP for child process - exiting");
			exit(EXIT_FAILURE);
		}

		else {
			log_info("caught SIGHUP for master process - "
					"reloading config files");
			reload();
		}

		break;

	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		if (getpid() != masterpid)
			log_info("caught SIGINT/SIGTERM/SIGQUIT for child process - "
					"ignoring");

		else {
			log_info("caught SIGINT/SIGTERM/SIGQUIT for master process - "
					"shutting down");
			exit(EXIT_SUCCESS);
		}

		break;

	case SIGSEGV:
		log_alert("caught SIGSEGV for virtual address %p (%d,%d)",
				info->si_addr, info->si_errno, info->si_code);
		log_alert("you probably found a bug in vcd!");
		log_alert("please report to %s", PACKAGE_BUGREPORT);
		exit(EXIT_FAILURE);
		break;
	}
}

int main(int argc, char **argv)
{
	int c, background = 0, debug = 0;

	/* free all memory on exit */
	atexit(mem_freeall);

	/* install signal handler */
	struct sigaction sa;

	sa.sa_sigaction = signal_handler;
	sa.sa_flags = SA_NODEFER|SA_SIGINFO;

	sigfillset(&sa.sa_mask);

	if (sigaction(SIGHUP, &sa, NULL) == -1 ||
			sigaction(SIGINT, &sa, NULL) == -1 ||
			sigaction(SIGQUIT, &sa, NULL) == -1 ||
			sigaction(SIGSEGV, &sa, NULL) == -1 ||
			sigaction(SIGTERM, &sa, NULL) == -1) {
		dprintf(STDERR_FILENO, "sigaction: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* parse command line */
	while ((c = getopt(argc, argv, "bc:d")) != -1) {
		switch (c) {
		case 'b':
			if (!debug)
				background = 1;
			break;

		case 'c':
			cfg_file = optarg;
			break;

		case 'd':
			background = 0;
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
	cfg_load(cfg_file);
	atexit(cfg_atexit);

	/* start logging & debugging */
	const char *logfile = cfg_getstr(cfg, "logfile");

	log_options.log_ident = argv[0];

	if (!str_isempty(logfile)) {
		log_options.log_dest |= LOGD_FILE;
		log_options.log_fd = open_append(logfile);
	}

	if (!background)
		log_options.log_dest |= LOGD_STDERR;

	if (debug)
		log_options.log_mask = ((1 << (LOGP_TRACE + 1)) - 1);

	log_init(&log_options);
	atexit(log_close);

	/* fork to background */
	if (background) {
		log_info("running in background mode");

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

	else if (debug)
		log_info("running in debugging mode");

	else
		log_info("running in foreground mode");

	/* daemonize */
	close(STDIN_FILENO);
	umask(0);
	setsid();
	chdir("/");

	/* log process id */
	int fd;
	const char *pidfile = cfg_getstr(cfg, "pidfile");

	masterpid = getpid();

	if (pidfile && (fd = open_trunc(pidfile)) != -1) {
		dprintf(fd, "%d\n", masterpid);
		close(fd);
	}

	/* initialize internal stats */
	init_stats();

	/* open connection to vxdb */
	vxdb_init();
	atexit(vxdb_atexit);

	/* setup listen socket */
	init_listen_socket();
	atexit(shutdown_listen_socket);

	/* setup xmlrpc server */
	xmlrpc_env _env; env = &_env;

	xmlrpc_env_init(env);
	init_server();

	/* never get here */
	return EXIT_FAILURE;
}
