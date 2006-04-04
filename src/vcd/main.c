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
#include <sys/stat.h>
#include <sys/wait.h>

#include "log.h"
#include "collector.h"
#include "server.h"

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
	
	openlog("vcd/master", LOG_CONS|LOG_PID, LOG_DAEMON);
	
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
		sleep(1);
		collector_main();
	
	default:
		pid = waitpid(collector, &status, WNOHANG);
		
		if (pid > 0 || (pid == -1 && errno == ECHILD)) {
			LOGWARN("unexpected death of controller");
			kill(0, SIGTERM);
		}
		
		if (pid == -1)
			LOGPERR("waitpid(controller)");
	}
	
	switch ((server = fork())) {
	case -1:
		LOGPERR("fork(server)");
	
	case 0:
		sleep(1);
		server_main();
	
	default:
		pid = waitpid(server, &status, WNOHANG);
		
		if (pid > 0 || (pid == -1 && errno == ECHILD)) {
			LOGWARN("unexpected death of server");
			kill(0, SIGTERM);
		}
		
		if (pid == -1)
			LOGPERR("waitpid(server)");
	}
	
wait:
	pid = waitpid(0, &status, 0);
	
	if (pid == -1) {
		if (errno == ECHILD)
			LOGERR("death of all children. following.");
		else {
			LOGPWARN("waitpid()");
			goto wait;
		}
	}
	
	else if (pid == collector)
		LOGWARN("collector died. following.");
	
	else if (pid == server)
		LOGWARN("server died. following.");
	
	kill(0, SIGTERM);
	return EXIT_FAILURE;
}
