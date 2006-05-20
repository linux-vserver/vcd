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
#include <sys/wait.h>
#include <arpa/inet.h>
#include <gnutls/gnutls.h>

#include "lucid.h"
#include "xmlrpc.h"
#include "xmlrpc_private.h"

#include "auth.h"
#include "cfg.h"
#include "log.h"
#include "vxdb.h"

#include "methods.h"

#define DH_BITS 1024

XMLRPC_SERVER xmlrpc_server;
static int sfd, cfd, num_clients;

typedef enum {
	TLS_DISABLED = 0,
	TLS_ANON     = 1,
	TLS_X509     = 2,
} tls_modes_t;

static tls_modes_t tls_mode = TLS_DISABLED;
static gnutls_anon_server_credentials_t anon;
static gnutls_certificate_credentials_t x509;
static gnutls_dh_params_t dh_params;
static gnutls_session_t tls_session;

typedef struct {
	int id;
	char *desc;
} httpd_status_t;

static
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
int httpd_read_line(char **line)
{
	int rc, chunks = 1, idx = 0;
	char *buf = malloc(chunks * CHUNKSIZE + 1);
	char c;

	for (;;) {
		if (tls_mode == TLS_DISABLED)
			rc = read(cfd, &c, 1);
		else
			rc = gnutls_record_recv(tls_session, &c, 1);
		
		if (rc < 0) {
			free(buf);
			return -1;
		}
		
		if (rc == 0) {
			idx = 0;
			goto out;
		}
		
		if (c == '\r')
			continue;
		
		if (c == '\n')
			goto out;
		
		if (idx >= chunks * CHUNKSIZE) {
			chunks++;
			buf = realloc(buf, chunks * CHUNKSIZE + 1);
		}
		
		buf[idx++] = c;
	}
	
out:
	buf[idx] = '\0';
	*line = buf;
	return strlen(buf);
}

static
int httpd_write(char *fmt, ...)
{
	char *buf;
	int rc;
	
	va_list ap;
	va_start(ap, fmt);
	
	vasprintf(&buf, fmt, ap);
	
	if (tls_mode == TLS_DISABLED)
		rc = write(cfd, buf, strlen(buf));
	else
		rc = gnutls_record_send(tls_session, buf, strlen(buf));
	
	free(buf);
	return rc;
}

static
void httpd_send_headers(int id, size_t clen)
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
	httpd_write("HTTP/1.1 %d %s\r\n", http_status_codes[i].id,
	                http_status_codes[i].desc);
	httpd_write("Server: VServer Control Daemon\r\n");
	httpd_write("Connection: close\r\n");
	
	if (clen > 0) {
		httpd_write("Content-Length: %d\r\n", clen);
		httpd_write("Content-Type: text/xml\r\n");
	}
	
	httpd_write("\r\n");
}

static
XMLRPC_VALUE call_method(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_Callback cb;
	
	if (r->error)
		return XMLRPC_CopyValue(r->error);
	
	if (!auth_isvalid(r))
		return method_error(MEAUTH);
	
	cb = XMLRPC_ServerFindMethod(s, r->methodName.str);
	
	if (cb)
		return cb(s, r, d);
	
	return XMLRPC_UtilityCreateFault(xmlrpc_error_unknown_method, r->methodName.str);
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
	XMLRPC_RequestSetData(response, call_method(xmlrpc_server, request, NULL));
	
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
		httpd_send_headers(503, 0);
		close(cfd);
		exit(EXIT_FAILURE);
	
	case SIGALRM:
		httpd_send_headers(408, 0);
		close(cfd);
		exit(EXIT_FAILURE);
	}
}

static
gnutls_session_t initialize_tls_session(void)
{
	int rc;
	gnutls_session_t session;
	const int kx_prio[] = { GNUTLS_KX_ANON_DH, 0 };
	
	if ((rc = gnutls_init(&session, GNUTLS_SERVER)) < 0)
		log_error_and_die("gnuttls_init: %s", gnutls_strerror(rc));
	
	gnutls_set_default_priority(session);
	
	if (tls_mode == TLS_ANON) {
		gnutls_kx_set_priority(session, kx_prio);
		
		if ((rc = gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon)) < 0)
			log_error_and_die("gnuttls_credentials_set: %s", gnutls_strerror(rc));
	}
	
	else if (tls_mode == TLS_X509) {
		if ((rc = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509)) < 0)
			log_error_and_die("gnuttls_credentials_set: %s", gnutls_strerror(rc));
		
		gnutls_certificate_server_set_request(session, GNUTLS_CERT_REQUEST);
	}
	
	gnutls_dh_set_prime_bits(session, DH_BITS);
	return session;
}

#define NOREPLACE(NEW) id == 0 ? NEW : id

static
void handle_client(void)
{
	int timeout = cfg_getint(cfg, "client-timeout");
	int rc, id = 0, clen = 0, first_line = 1;
	char *line = NULL, *req = NULL, *res = NULL;
	
	/* setup some standard signals */
	signal(SIGTERM, client_signal_handler);
	
	/* timeout */
	signal(SIGALRM, client_signal_handler);
	alarm(timeout);
	
	/* init TLS */
	if (tls_mode != TLS_DISABLED) {
		tls_session = initialize_tls_session();
		
		gnutls_transport_set_ptr(tls_session, (gnutls_transport_ptr_t) cfd);
		
		if ((rc = gnutls_handshake(tls_session)) < 0) {
			gnutls_deinit(tls_session);
			log_error_and_die("Handshake has failed: %s", gnutls_strerror(rc));
		}
	}
	
	/* parse request header */
	while (1) {
		if (line != NULL)
			free(line);
		
		rc = httpd_read_line(&line);
		
		if (rc == -1)
			goto close;
		
		else if (rc == 0)
			break;
		
		/* parse request line */
		else if (first_line) {
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
			if (strncmp(line, "Content-Length: ", 16) == 0)
				clen = atoi(line + 16);
		}
	}
	
	/* remove timeout */
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	
	/* an error occured and id was set */
	if (id > 0) {
		httpd_send_headers(id, 0);
		goto close;
	}
	
	/* no XMLRPC request was sent */
	if (clen <= 0) {
		httpd_send_headers(400, 0);
		goto close;
	}
	
	/* get XMLRPC request */
	if (tls_mode == TLS_DISABLED)
		rc = io_read_len(cfd, &req, clen);
	
	else {
		req = calloc(clen + 1, sizeof(char));
		rc  = gnutls_record_recv(tls_session, req, clen);
	}
	
	/* connection died? */
	if (rc == -1)
		goto close;
	
	/* invalid request length */
	if (rc != clen) {
		httpd_send_headers(400, 0);
		goto close;
	}
	
	/* handle request */
	rc = handle_request(req, &res);
	
	free(req);
	
	/* invalid XML */
	if (rc == -1)
		httpd_send_headers(400, 0);
	
	/* somehow the response didn't build */
	else if (res == NULL)
		httpd_send_headers(500, 0);
	
	/* send response */
	else {
		clen = strlen(res);
		httpd_send_headers(200, clen);
		httpd_write(res, clen);
		free(res);
	}
	
close:
	gnutls_bye(tls_session, GNUTLS_SHUT_WR);
	close(cfd);
	gnutls_deinit(tls_session);
	exit(0);
}

static
void server_signal_handler(int sig, siginfo_t *siginfo, void *u)
{
	switch (sig) {
	case SIGCHLD:
		num_clients--;
		wait(NULL);
		break;
	
	case SIGTERM:
		log_info("Caught SIGTERM -- shutting down");
		
		signal(SIGCHLD, SIG_IGN);
		kill(0, SIGTERM);
		
		close(sfd);
		
		vxdb_close();
		
		gnutls_certificate_free_credentials(x509);
		gnutls_global_deinit();
		
		exit(EXIT_FAILURE);
	}
}

void server_main(void)
{
	int rc, port, i, max_clients;
	char *host, peer[INET_ADDRSTRLEN];
	socklen_t peerlen;
	struct sockaddr_in host_addr, peer_addr;
	struct sigaction act;
	char *ca, *crl, *cert, *key;
	
	log_init("server", 0);
	log_info("Loading configuration");
	
	tls_mode = cfg_getint(cfg, "tls-mode");
	
	if (tls_mode == TLS_ANON) {
		gnutls_global_init();
		gnutls_anon_allocate_server_credentials(&anon);
		
		if ((rc = gnutls_dh_params_init(&dh_params)) < 0)
			log_error_and_die("gnuttls_dh_params_init: %s", gnutls_strerror(rc));
		
		if ((rc = gnutls_dh_params_generate2(dh_params, DH_BITS)) < 0)
			log_error_and_die("gnuttls_dh_params_generate2: %s", gnutls_strerror(rc));
		
		gnutls_anon_set_server_dh_params(anon, dh_params);
		
		log_info("TLS with anonymous authentication configured successfully");
	}
	
	else if (tls_mode == TLS_X509) {
		key  = cfg_getstr(cfg, "tls-server-key");
		cert = cfg_getstr(cfg, "tls-server-crt");
		crl  = cfg_getstr(cfg, "tls-server-crl");
		ca   = cfg_getstr(cfg, "tls-ca-crt");
		
		if (!key || !cert)
			log_error_and_die("No TLS key or certificate specified");
		
		gnutls_global_init();
		
		if ((rc = gnutls_certificate_allocate_credentials(&x509)) < 0)
			log_error_and_die("gnuttls_certificate_allocate_credentials: %s", gnutls_strerror(rc));
		
		if ((rc = gnutls_certificate_set_x509_key_file(x509, cert, key, GNUTLS_X509_FMT_PEM)) < 0)
			log_error_and_die("gnuttls_certificate_set_x509_key_file: %s", gnutls_strerror(rc));
			
		if (ca && (rc = gnutls_certificate_set_x509_trust_file(x509, ca, GNUTLS_X509_FMT_PEM)) < 0)
			log_error_and_die("gnuttls_certificate_set_x509_trust_file: %s", gnutls_strerror(rc));
		
		if (crl && (rc = gnutls_certificate_set_x509_crl_file(x509, crl, GNUTLS_X509_FMT_PEM)) < 0)
			log_error_and_die("gnuttls_certificate_set_x509_crl_file: %s", gnutls_strerror(rc));
		
		if ((rc = gnutls_dh_params_init(&dh_params)) < 0)
			log_error_and_die("gnuttls_dh_params_init: %s", gnutls_strerror(rc));
		
		if ((rc = gnutls_dh_params_generate2(dh_params, DH_BITS)) < 0)
			log_error_and_die("gnuttls_dh_params_generate2: %s", gnutls_strerror(rc));
		
		gnutls_certificate_set_dh_params(x509, dh_params);
		
		log_info("TLS with X.509 authentication configured successfully");
	}
	
	/* open connection to vxdb */
	vxdb_init();
	
	port = cfg_getint(cfg, "listen-port");
	host = cfg_getstr(cfg, "listen-host");
	
	bzero(&host_addr, sizeof(host_addr));
	
	host_addr.sin_family = AF_INET;
	host_addr.sin_port   = htons(port);
	
	if (inet_pton(AF_INET, host, &host_addr.sin_addr) < 1) {
		log_warn("Invalid configuration for listen-host: %s", host);
		log_info("Using fallback listen-host: 127.0.0.1");
		inet_pton(AF_INET, "127.0.0.1", &host_addr.sin_addr);
	}
	
	max_clients = cfg_getint(cfg, "client-max");
	
	/* handle these signals on our own */
	sigfillset(&act.sa_mask);
	
	act.sa_flags     = SA_SIGINFO;
	act.sa_sigaction = server_signal_handler;
	
	sigaction(SIGCHLD, &act, NULL);
	
	sigdelset(&act.sa_mask, SIGCHLD);
	sigaction(SIGTERM, &act, NULL);
	
	/* setup listen socket */
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sfd == -1)
		log_error_and_die("socket: %s", strerror(errno));
	
	if (bind(sfd, (struct sockaddr *) &host_addr, sizeof(struct sockaddr_in)) == -1)
		log_error_and_die("bind: %s", strerror(errno));
	
	if (listen(sfd, 20) == -1)
		log_error_and_die("listen: %s", strerror(errno));
	
	/* setup xmlrpc server */
	xmlrpc_server = XMLRPC_ServerCreate();
	method_registry_init(xmlrpc_server);
	
	log_info("Accepting incoming connections on %s:%d", host, port);
	
	/* wait and create a new child for each connection */
	while (1) {
		peerlen = sizeof(struct sockaddr_in);
		cfd = accept(sfd, (struct sockaddr *) &peer_addr, &peerlen);
		
		if (cfd == -1) {
			if (errno != EINTR)
				log_warn("accept: %s", strerror(errno));
			
			continue;
		}
		
		if (num_clients >= max_clients) {
			log_warn("Maximum number of connections reached");
			log_info("Rejecting client from %s:%d",
			         inet_ntop(AF_INET, &peer_addr.sin_addr, peer, INET_ADDRSTRLEN),
			         ntohs(peer_addr.sin_port));
			httpd_send_headers(503, 0);
			close(cfd);
			continue;
		}
		
		switch (fork()) {
		case -1:
			log_warn("fork: %s", strerror(errno));
			break;
		
		case 0:
			log_info("New connection from %s, port %d",
			         inet_ntop(AF_INET, &peer_addr.sin_addr, peer, INET_ADDRSTRLEN),
			         ntohs(peer_addr.sin_port));
			close(sfd);
			handle_client();
			break;
		
		default:
			num_clients++;
			close(cfd);
			break;
		}
	}
}
