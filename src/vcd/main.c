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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "log.h"

void collector_main(void)
{
	while(1) sleep(1);
}

void server_main(void)
{
	while(1) sleep(1);
}

int main(int argc, char **argv)
{
	pid_t pid, server, collector;
	int status;
	
	switch (fork()) {
	case -1:
		perror("fork()");
		exit(EXIT_FAILURE);
	
	case 0:
		break;
	
	default:
		exit(EXIT_SUCCESS);
	}
	
	openlog(argv[0], LOG_CONS|LOG_PID, LOG_DAEMON);
	
	umask(0);
	setsid();
	
	if (chdir("/"))
		LOGPERR("chdir(/)");
	
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	
	switch ((collector = fork())) {
	case -1:
		LOGPERR("fork(collector)");
	
	case 0:
		collector_main();
	
	default:
		pid = waitpid(collector, &status, WNOHANG);
		
		if (pid > 0 || (pid == -1 && errno == ECHILD))
			LOGERR("unexpected death of controller");
		
		if (pid == -1)
			LOGPERR("waitpid(controller)");
	}
	
	switch ((server = fork())) {
	case -1:
		LOGPERR("fork(server)");
	
	case 0:
		server_main();
	
	default:
		pid = waitpid(server, &status, WNOHANG);
		
		if (pid > 0 || (pid == -1 && errno == ECHILD))
			LOGERR("unexpected death of server");
		
		if (pid == -1)
			LOGPERR("waitpid(server)");
	}
	
wait:
	pid = waitpid(0, &status, 0);
	
	if (pid == -1) {
		if (errno == ECHILD)
			LOGERR("unexpected death of all children");
		else
			LOGPWARN("waitpid()");
			goto wait; /* loop? */
	}
	
	if (pid > 0) {
		LOGWARN("unexpected death of child. following..");
		kill(0, SIGTERM);
	}
	
	return EXIT_FAILURE; /* never get here */
}
