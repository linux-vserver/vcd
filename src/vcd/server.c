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

#include "auth.h"
#include "cfg.h"
#include "log.h"
#include "methods.h"
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

static
size_t httpd_read(void *src, char *data, size_t len)
{
	if (tls_mode == TLS_DISABLED)
		return read(*(int *) src, data, len);
	else
		return gnutls_record_send(*(gnutls_session_t *) src, data, len);
}

static
size_t httpd_write(void *dst, char *data, size_t len)
{
	if (tls_mode == TLS_DISABLED)
		return write(*(int *) dst, data, len);
	else
		return gnutls_record_send(*(gnutls_session_t *) dst, data, len);
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
	XMLRPC_RequestSetData(response, method_call(xmlrpc_server, request, NULL));
	
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
		close(cfd);
		exit(EXIT_FAILURE);
	
	case SIGALRM:
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

static
void handle_client(void)
{
	int timeout = cfg_getint(cfg, "client-timeout");
	int rc;
	
	/* setup some standard signals */
	signal(SIGTERM, client_signal_handler);
	
	/* timeout */
	signal(SIGALRM, client_signal_handler);
	alarm(timeout);
	
	void *src;
	
	/* init TLS */
	if (tls_mode != TLS_DISABLED) {
		tls_session = initialize_tls_session();
		
		gnutls_transport_set_ptr(tls_session, (gnutls_transport_ptr_t) cfd);
		
		if ((rc = gnutls_handshake(tls_session)) < 0) {
			gnutls_deinit(tls_session);
			log_error_and_die("Handshake has failed: %s", gnutls_strerror(rc));
		}
		
		src = (void *) &tls_session;
	}
	
	else
		src = (void *) &cfd;
	
	http_request_t request;
	http_header_t *headers = (http_header_t *) malloc(sizeof(http_header_t));
	char *body;
	
	if (http_get_request(src, &request, headers, &body, httpd_read) == -1)
		log_error_and_die("Could not get request: %s", strerror(errno));
	
	http_headers_free(headers);
	
	/* remove timeout */
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	
	http_response_t response;
	http_header_t *tmp;
	char *rbody = NULL;
	
	response.status = HTTP_STATUS_BADREQ;
	response.vmajor = 1;
	response.vminor = 0;
	
	headers = (http_header_t *) malloc(sizeof(http_header_t));
	INIT_LIST_HEAD(&(headers->list));
	
	if (request.method != HTTP_METHOD_POST || strcmp(request.uri, "/RPC2") != 0) {
		http_send_response(src, &response, headers, NULL, httpd_write);
		goto close;
	}
	
	rc = handle_request(body, &rbody);
	
	if (!rbody)
		response.status = HTTP_STATUS_INTERNAL;
	
	else {
		response.status = HTTP_STATUS_OK;
		
		tmp = (http_header_t *) malloc(sizeof(http_header_t));
		asprintf(&(tmp->key), "Content-Length");
		asprintf(&(tmp->val), "%d", strlen(rbody));
		
		list_add(&(tmp->list), &(headers->list));
	}
	
	http_send_response(src, &response, headers, rbody, httpd_write);
	free(rbody);
	
close:
	http_headers_free(headers);
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
	
	gnutls_global_init();
	
	if (tls_mode == TLS_ANON) {
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
