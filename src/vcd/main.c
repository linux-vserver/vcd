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

#include "lucid.h"

#include "cfg.h"
#include "log.h"

static cfg_opt_t CFG_OPTS[] = {
	/* network configuration */
	CFG_STR_CB("listen-host", "127.0.0.1", CFGF_NONE, &cfg_validate_host),
	CFG_INT_CB("listen-port", 13386,       CFGF_NONE, &cfg_validate_port),
	
	/* SSL/TLS */
	CFG_INT_CB("tls-mode",       0,    CFGF_NONE, &cfg_validate_tls),
	CFG_STR_CB("tls-server-key", NULL, CFGF_NONE, &cfg_validate_path),
	CFG_STR_CB("tls-server-crt", NULL, CFGF_NONE, &cfg_validate_path),
	CFG_STR_CB("tls-server-crl", NULL, CFGF_NONE, &cfg_validate_path),
	CFG_STR_CB("tls-ca-crt",     NULL, CFGF_NONE, &cfg_validate_path),
	
	/* client handling */
	CFG_INT("client-max",     20, CFGF_NONE),
	CFG_INT("client-timeout", 30, CFGF_NONE),
	
	/* logging */
	CFG_INT_CB("log-level", 3,    CFGF_NONE, &cfg_validate_log),
	
	/* filesystem layout */
	CFG_STR_CB("vxdb-dir",     NULL, CFGF_NONE, &cfg_validate_path),
	CFG_STR_CB("lock-dir",     NULL, CFGF_NONE, &cfg_validate_path),
	CFG_STR_CB("log-dir",      NULL, CFGF_NONE, &cfg_validate_path),
	CFG_STR_CB("run-dir",      NULL, CFGF_NONE, &cfg_validate_path),
	CFG_STR_CB("vserver-dir",  NULL, CFGF_NONE, &cfg_validate_path),
	CFG_STR_CB("template-dir", NULL, CFGF_NONE, &cfg_validate_path),
	CFG_END()
};

cfg_t *cfg;

void collector_main(void);
void server_main(void);

static pid_t server, collector;

static inline
void usage(int rc)
{
	printf("Usage: vcd [<opts>*]\n"
	       "\n"
	       "Available options:\n"
	       "   -c <file>     configuration file (default: /etc/vcd.conf)\n"
	       "   -d            debug mode (do not fork to background)\n");
	exit(rc);
}

static
void signal_handler(int sig, siginfo_t *siginfo, void *u)
{
	int i, status;
	
	switch (sig) {
	case SIGCHLD:
		if (siginfo->si_pid == collector)
			log_error("Unexpected death of collector");
		
		else if (siginfo->si_pid == server)
			log_error("Unexpected death of server");
		
		else
			log_warn("Caught SIGCHLD for unknown PID %d", siginfo->si_pid);
		
		break;
	
	case SIGINT:
		log_info("Interrupt from keyboard -- shutting down");
		break;
	
	case SIGTERM:
		log_info("Caught SIGTERM -- shutting down");
		break;
	}
	
	kill(0, SIGTERM);
	
	for (i = 0; i < 5; i++) {
		if (wait(NULL) == -1 && errno == ECHILD)
			break;
		
		usleep(200);
		
		if (i == 4) {
			kill(0, SIGKILL);
			i = 0;
		}
	}
	
	if (cfg)
		cfg_free(cfg);
	
	exit(EXIT_FAILURE);
}

static
void reset_signals(void)
{
	signal(SIGINT,  SIG_IGN);
	signal(SIGCHLD, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
}

int main(int argc, char **argv)
{
	char *cfg_file = "/etc/vcd.conf";
	char *rundir, *pidfile;
	pid_t pid = 0;
	int fd, c, status, debug = 0;
	struct sigaction act;
	
	/* getopt */
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
	
	/* start logging & debugging */
	if (log_init("master", debug) == -1) {
		perror("log_init");
		exit(EXIT_FAILURE);
	}
	
	/* fork to background */
	if (!debug)
		pid = fork();
	
	switch (pid) {
	case -1:
		log_error_and_die("fork: %s", strerror(errno));
	
	case 0:
		break;
	
	default:
		exit(EXIT_SUCCESS);
	}
	
	/* daemon stuff */
	umask(0);
	setsid();
	chdir("/");
	
	close(STDIN_FILENO);
	
	if (!debug) {
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}
	
	/* ignore some standard signals */
	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGABRT, SIG_IGN);
	
	if (!debug)
		signal(SIGINT,  SIG_IGN);
	
	/* handle these signals on our own */
	sigfillset(&act.sa_mask);
	
	act.sa_flags     = SA_SIGINFO;
	act.sa_sigaction = signal_handler;
	
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGCHLD, &act, NULL);
	
	if (debug)
		sigaction(SIGINT, &act, NULL);
	
	/* log process id */
	rundir  = cfg_getstr(cfg, "run-dir");
	
	if (rundir) {
		asprintf(&pidfile, "%s/vcd.pid", rundir);
		
		if ((fd = open_trunc(pidfile)) != -1) {
			dprintf(fd, "%d\n", getpid());
			close(fd);
		}
		
		free(pidfile);
	}
	
	/* start threads */
	log_info("Starting collector thread");
	
	switch ((collector = fork())) {
	case -1:
		log_error_and_die("fork: %s", strerror(errno));
	
	case 0:
		reset_signals();
		collector_main();
	
	default:
		break;
	}
	
	log_info("Starting XMLRPC server thread");
	
	switch ((server = fork())) {
	case -1:
		log_error_and_die("fork: %s", strerror(errno));
	
	case 0:
		reset_signals();
		server_main();
	
	default:
		break;
	}
	
	log_info("Resuming normal operation");
	
	while (1)
		sleep(1);
}
