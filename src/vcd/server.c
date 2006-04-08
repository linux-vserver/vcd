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
#include <sys/socket.h>
#include <arpa/inet.h>

#include "pathconfig.h"

#include "lucid.h"
#include "confuse.h"
#include "xmlrpc.h"

#include "log.h"
#include "cfg.h"

#include "methods/methods.h"

static XMLRPC_SERVER xmlrpc_server;
static cfg_t *cfg, *cfg_listen, *cfg_server;
static int cfd, num_clients;

typedef struct {
	int id;
	char *desc;
} httpd_status_t;

httpd_status_t http_status_codes[] = {
	{ 100, "Continue" },
	{ 101, "Switching Protocols" },
	{ 200, "OK" },
	{ 201, "Created" },
	{ 202, "Accepted" },
	{ 203, "Non-Authoritative Information" },
	{ 204, "No Content" },
	{ 205, "Reset Content" },
	{ 206, "Partial Content" },
	{ 300, "Multiple Choices" },
	{ 301, "Moved Permanently" },
	{ 302, "Found" },
	{ 303, "See Other" },
	{ 304, "Not Modified" },
	{ 305, "Use Proxy" },
	{ 307, "Temporary Redirect" },
	{ 400, "Bad Request" },
	{ 401, "Unauthorized" },
	{ 402, "Payment Required" },
	{ 403, "Forbidden" },
	{ 404, "Not Found" },
	{ 405, "Method Not Allowed" },
	{ 406, "Not Acceptable" },
	{ 407, "Proxy Authentication Required" },
	{ 408, "Request Timeout" },
	{ 409, "Conflict" },
	{ 410, "Gone" },
	{ 411, "Length Required" },
	{ 412, "Precondition Failed" },
	{ 413, "Request Entity Too Large" },
	{ 414, "Request-URI Too Long" },
	{ 415, "Unsupported Media Type" },
	{ 416, "Requested Range Not Satisfiable" },
	{ 417, "Expectation Failed" },
	{ 500, "Internal Server Error" },
	{ 501, "Not Implemented" },
	{ 502, "Bad Gateway" },
	{ 503, "Service Unavailable" },
	{ 504, "Gateway Timeout" },
	{ 505, "HTTP Version Not Supported" },
	{ 0,   NULL }
};

static
int httpd_read_line(int fd, char **line)
{
	int chunks = 1, idx = 0;
	char *buf = malloc(chunks * CHUNKSIZE + 1);
	char c;

	for (;;) {
		switch(read(fd, &c, 1)) {
			case -1:
				return -1;
			
			case 0:
				return -2;
			
			default:
				if (c == '\r')
					break;
				
				if (c == '\n')
					goto out;
				
				if (idx >= chunks * CHUNKSIZE) {
					chunks++;
					buf = realloc(buf, chunks * CHUNKSIZE + 1);
				}
				
				buf[idx++] = c;
				break;
		}
	}
	
out:
	buf[idx] = '\0';
	*line = buf;
	return strlen(buf);
}

static
void httpd_send_headers(int fd, int id, size_t clen)
{
	static int headers_sent = 0;
	int i;
	
	if (headers_sent == 1)
		return;
	
	headers_sent = 1;
	
	/* find description */
	for (i = 0;; i++) {
		if (http_status_codes[i].id == 0) {
			id = 500;
			i  = 0;
			continue;
		}
		
		if (http_status_codes[i].id == id)
			goto send;
	}
	
send:
	dprintf(fd, "HTTP/1.1 %d %s\r\n", http_status_codes[i].id,
	        http_status_codes[i].desc);
	dprintf(fd, "Server: VServer Control Daemon\r\n");
	dprintf(fd, "Connection: close\r\n");
	
	if (clen > 0) {
		dprintf(fd, "Content-Length: %d\r\n", clen);
		dprintf(fd, "Content-Type: text/xml\r\n");
	}
	
	dprintf(fd, "\r\n");
}

static
int handle_request(char *req, char **res)
{
	XMLRPC_REQUEST request, response;
	char *buf;
	
	/* parse XML */
	size_t len = strlen(req);
	request = XMLRPC_REQUEST_FromXML(req, len, NULL);
	
	if (!request)
		return -1;
	
	/* create a response struct */
	response = XMLRPC_RequestNew();
	XMLRPC_RequestSetRequestType(response, xmlrpc_request_response);
	
	/* call requested method and fill response struct */
	XMLRPC_RequestSetData(response,
	                      XMLRPC_ServerCallMethod(xmlrpc_server, request, NULL));
	
	/* reply in same vocabulary/manner as the request */
	XMLRPC_RequestSetOutputOptions(response,
	                               XMLRPC_RequestGetOutputOptions(request));
	
	/* serialize server response as XML */
	buf = XMLRPC_REQUEST_ToXML(response, 0);
	
	if (buf)
		*res = buf;
	
	XMLRPC_RequestFree(request, 1);
	XMLRPC_RequestFree(response, 1);
	
	return 0;
}

static
void client_signal_handler(int sig)
{
	switch (sig) {
	case SIGTERM:
		httpd_send_headers(cfd, 503, 0);
		close(cfd);
		exit(EXIT_FAILURE);
	
	case SIGALRM:
		httpd_send_headers(cfd, 408, 0);
		close(cfd);
		exit(EXIT_FAILURE);
	}
}

#define NOREPLACE(NEW) id == 0 ? NEW : id

static
void handle_client(int cfd)
{
	int timeout = cfg_getint(cfg_server, "timeout");
	int rc, id = 0, clen = 0, first_line = 1;
	char *line = NULL, *req = NULL, *res = NULL;
	
	/* setup some standard signals */
	signal(SIGHUP,  SIG_IGN);
	signal(SIGINT,  SIG_IGN);
	signal(SIGTERM, client_signal_handler);
	
	/* timeout */
	signal(SIGALRM, client_signal_handler);
	alarm(timeout);
	
	/* parse request header */
	while (1) {
		if (line != NULL)
			free(line);
		
		rc = httpd_read_line(cfd, &line);
		
		/* EOF before EOL */
		if (rc == -2)
			id = NOREPLACE(400);
		
		/* unknown error */
		else if (rc == -1)
			goto close;
		
		/* parse request line */
		else if (first_line) {
			if (rc == 0)
				continue;
			else
				first_line = 0;
			
			/* only support POST request */
			if (strncmp(line, "POST", 4) != 0)
				id = NOREPLACE(501);
			
			/* only support requests via /RPC2 */
			else if (strncmp(line + 5, "/RPC2", 5) != 0)
				id = NOREPLACE(404);
		}
		
		/* parse additional headers */
		else {
			if (rc == 0) {
				free(line);
				break;
			}
			
			/* we need to know Content-Length */
			else if (strncmp(line, "Content-Length: ", 16) == 0)
				clen = atoi(line + 16);
		}
	}
	
	/* remove timeout */
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	
	/* an error occured and id was set */
	if (id > 0) {
		httpd_send_headers(cfd, id, 0);
		goto close;
	}
	
	/* no XMLRPC request was sent */
	if (clen <= 0) {
		httpd_send_headers(cfd, 400, 0);
		goto close;
	}
	
	/* get XMLRPC request */
	rc = io_read_len(cfd, &req, clen);
	
	/* connection died? */
	if (rc == -1)
		goto close;
	
	/* invalid request length */
	if (rc != clen) {
		httpd_send_headers(cfd, 400, 0);
		goto close;
	}
	
	/* handle request */
	rc = handle_request(req, &res);
	
	free(req);
	
	/* invalid XML */
	if (rc == -1)
		httpd_send_headers(cfd, 400, 0);
	
	/* somehow the response didn't build */
	else if (res == NULL)
		httpd_send_headers(cfd, 500, 0);
	
	/* send response */
	else {
		clen = strlen(res);
		httpd_send_headers(cfd, 200, clen);
		write(cfd, res, clen);
		free(res);
	}
	
close:
	close(cfd);
	exit(0);
}

static
void server_signal_handler(int sig)
{
	switch (sig) {
	case SIGCHLD:
		num_clients--;
		break;
	
	case SIGTERM:
		kill(0, SIGTERM);
		exit(EXIT_FAILURE);
	}
}

void server_main(void)
{
	int port, sfd, i, max_clients;
	char *host;
	struct sockaddr_in addr;
	
	/* open connection to syslog */
	openlog("vcd/server", LOG_CONS|LOG_PID, LOG_DAEMON);
	
	/* load configuration */
	cfg = cfg_init(CFG, CFGF_NOCASE);
	
	switch (cfg_parse(cfg, __PKGCONFDIR "/vcd.conf")) {
	case CFG_FILE_ERROR:
		LOGPERR("vcd.conf");
	
	case CFG_PARSE_ERROR:
		LOGERR("vcd.conf: parse error");
	
	default:
		break;
	}
	
	cfg_listen = cfg_getsec(cfg, "listen");
	cfg_server = cfg_getsec(cfg, "server");
	
	port = cfg_getint(cfg_listen, "port");
	host = cfg_getstr(cfg_listen, "host");
	
	max_clients = cfg_getint(cfg_server, "max-clients");
	
	/* setup xmlrpc server */
	xmlrpc_server = XMLRPC_ServerCreate();
	register_methods(xmlrpc_server);
	
	/* setup some standard signals */
	signal(SIGHUP,  SIG_IGN);
	signal(SIGINT,  SIG_IGN);
	signal(SIGTERM, server_signal_handler);
	signal(SIGCHLD, server_signal_handler);
	
	/* setup listen socket */
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sfd == -1)
		LOGPERR("socket");
	
	bzero(&addr, sizeof(addr));
	
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	
	if (!inet_pton(AF_INET, host, &addr.sin_addr)) {
		LOGWARN("invalid listen host. using fallback");
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	}
	
	if (bind(sfd, (struct sockaddr *) &addr,
	    sizeof(struct sockaddr_in)) == -1)
		LOGPERR("bind");
	
	if (listen(sfd, cfg_getint(cfg_listen, "backlog")) == -1)
		LOGPERR("listen");
	
	/* wait and create a new child for each connection */
	while (1) {
		cfd = accept(sfd, NULL, 0);
		
		if (cfd == -1) {
			LOGPWARN("accept");
			continue;
		}
		
		if (num_clients >= max_clients) {
			httpd_send_headers(cfd, 503, 0);
			close(cfd);
			continue;
		}
		
		switch (fork()) {
		case -1:
			LOGPWARN("fork");
			break;
		
		case 0:
			close(sfd);
			handle_client(cfd);
			break;
		
		default:
			num_clients++;
			close(cfd);
			break;
		}
	}
}
