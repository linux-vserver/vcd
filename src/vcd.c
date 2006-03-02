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

#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <wait.h>
#include <sys/poll.h>
#include <syslog.h>
#include <errno.h>
#include <arpa/inet.h>
#include <lucid/argv.h>
#include <lucid/io.h>

#include "vc.h"
#include "commands.h"

static const char *rcsid = "$Id$";

struct request_t {
	char *str;
	int id;
} REQ[] = {
#define VCD_HELO 0x1
	{ "HELO",   0x1 },
#define VCD_PASSWD 0x2
	{ "PASSWD", 0x2 },
#define VCD_GET 0x3
	{ "GET",    0x3 },
#define VCD_SET 0x4
	{ "SET",    0x4 },
#define VCD_CALL 0x5
	{ "CALL",   0x5 },
#define VCD_BYE 0x6
	{ "BYE",    0x6 },
	{ NULL,     0x0 },
};

#define VCDR_REQ_INVALID   99
#define VCDR_AUTH_OK      100
#define VCDR_AUTH_USER    101
#define VCDR_AUTH_PASSWD  102
#define VCDR_AUTH_CHGD    103
#define VCDR_AUTH_NEEDED  199
#define VCDR_KEY_FOUND    200
#define VCDR_KEY_WRITTEN  201
#define VCDR_KEY_FAIL     299
#define VCDR_EXIT_SUCCESS 300
#define VCDR_EXIT_FAIL    301
#define VCDR_CMD_FAIL     399

int str_to_request(char *str)
{
	int i;
	
	for (i = 0; REQ[i].str; i++)
		if (strcmp(str, REQ[i].str) == 0)
			return REQ[i].id;
	
	return 0;
}

void return_code(int fd, int status, int len)
{
	vc_dprintf(fd, "%03d", status);
	
	if (len > 0)
		vc_dprintf(fd, " %d\n", len);
	else
		vc_dprintf(fd, "\n");
}

void return_data(int fd, char **data)
{
	if (*data == NULL)
		return;
	
	if (strlen(*data) < 1)
		goto out;
	
	vc_dprintf(fd, "%s", *data);

out:
	free(*data);
	*data = NULL;
}

int handle_request(int fd, char *line, char **data)
{
	int req, ac;
	char **av = NULL;
	
	*data = NULL;
	
	argv_parse(line, &ac, &av);
	req = str_to_request(av[0]);
	
	int rc;
	char *buf;
	
	switch (req) {
		case VCD_HELO:
		case VCD_PASSWD:
		case VCD_SET:
		case VCD_GET:
			return VCDR_REQ_INVALID;
		
		case VCD_CALL:
			rc = do_command(ac-1, av+1, &buf);
			
			if (rc == -1)
				return VCDR_CMD_FAIL;
			
			*data = buf;
			
			if (rc == EXIT_SUCCESS)
				return VCDR_EXIT_SUCCESS;
			
			else
				return VCDR_EXIT_FAIL;
		
		case VCD_BYE:
			close(fd);
			_exit(EXIT_SUCCESS);
			
		default:
			return VCDR_REQ_INVALID;
	}
	
	/* we should never get here */
	return VCDR_REQ_INVALID;
}


void handle_client(int fd)
{
	/* 1) say hello */
	vc_dprintf(fd, "000 VCD/%s Welcome stranger\n", PACKAGE_VERSION);
	
	/* 2) handle request */
	int len, rc = VCDR_REQ_INVALID;
	char *line = NULL;
	char *data = NULL;
	
	while (1) {
		len = io_read_eol(fd, &line);
		
		if (len == -1)
			_exit(EXIT_FAILURE);
		
		if (len < 1)
			rc = VCDR_REQ_INVALID;
		
		else
			rc = handle_request(fd, line, &data);
		
		if (data == NULL)
			len = 0;
		else
			len = strlen(data);
		
		free(line);
		return_code(fd, rc, len);
		return_data(fd, &data);
	}
}

void sighandler(int sig)
{
	int status;
	
	switch (sig) {
		case SIGCHLD:
			wait(&status);
			break;
	}
}

int main(int argc, char *argv[])
{
	VC_INIT_ARGV0
	
	/* 1) load configuration */
	char *ip;
	int port;
	
	if (vc_cfg_get_str(NULL, "vcd.ip", &ip) == -1)
		vc_errp("vc_cfg_get_str(vcd.ip)");
	
	if (vc_cfg_get_int(NULL, "vcd.port", &port) == -1)
		vc_errp("vc_cfg_get_str(vcd.port)");
	
	/* 2) daemonize */
	pid_t pid, sid;
	
	/* 2a) fork off the parent process */
	if ((pid = fork()) == -1)
		vc_errp("fork");
	
	if (pid > 0)
		exit(EXIT_SUCCESS);
	
	/* 2b) open connection to syslog */
	openlog("vcd", LOG_CONS|LOG_PID, LOG_DAEMON);
	
	/* 2c) create a new session id for out child */
	if ((sid = setsid()) == -1) {
		syslog(LOG_ERR, "setsid: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	/* 2d) change current working directory */
	if ((chdir("/")) == -1) {
		syslog(LOG_ERR, "chdir: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	/* 2e) close standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	
	/* 2f) signal handler */
	signal(SIGCHLD, sighandler);
	
	/* 3) open listen socket */
	int sfd;
	struct sockaddr_in addr;
	
	if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		syslog(LOG_ERR, "socket: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	memset(&addr, 0, sizeof(struct sockaddr_in));
	
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	inet_aton(ip, &(addr.sin_addr));
	
	if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr)) == -1) {
		syslog(LOG_ERR, "bind: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	if (listen(sfd, 10) == -1)  {
		syslog(LOG_ERR, "listen: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	/* 4) wait for connections */
	int cfd;
	
	while (1) {
		struct sockaddr_in client;
		socklen_t clientlen = sizeof(client);
		
		if ((cfd = accept(sfd, (struct sockaddr *) &client, &clientlen)) == -1) {
			syslog(LOG_ERR, "accept: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		
		syslog(LOG_INFO, "New connection from %s", inet_ntoa(addr.sin_addr));
		
		switch (fork()) {
			case -1:
				close(cfd);
				break;
			
			case 0:
				handle_client(cfd);
			
			default:
				close(cfd);
		}
	}
	
	/* never get here */
	syslog(LOG_CRIT, "Unexpected death of daemon");
	return EXIT_FAILURE;
}
