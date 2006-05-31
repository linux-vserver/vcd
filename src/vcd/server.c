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

#include "cfg.h"
#include "log.h"
#include "methods.h"
#include "vxdb.h"

#define DH_BITS 1024

XMLRPC_SERVER xmlrpc_server;
static int sfd, cfd, num_clients, max_clients;
static struct sockaddr_in peer_addr;

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
void client_signal_handler(int sig)
{
	switch (sig) {
	case SIGTERM:
	case SIGALRM:
		gnutls_bye(tls_session, GNUTLS_SHUT_WR);
		close(cfd);
		gnutls_deinit(tls_session);
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
	gnutls_transport_set_ptr(session, (gnutls_transport_ptr_t) cfd);
	
	if ((rc = gnutls_handshake(session)) < 0) {
		gnutls_deinit(session);
		log_error_and_die("Handshake has failed: %s", gnutls_strerror(rc));
	}
	
	return session;
}

static
void handle_client(void)
{
	void *src;
	http_request_t request;
	http_response_t response;
	http_header_t *headers, *tmp;
	char *body = NULL, *rbody = NULL;
	char peer[INET_ADDRSTRLEN];
	
	int timeout = cfg_getint(cfg, "client-timeout");
	
	/* setup some standard signals */
	signal(SIGTERM, client_signal_handler);
	
	/* timeout */
	signal(SIGALRM, client_signal_handler);
	alarm(timeout);
	
	/* init TLS */
	if (tls_mode != TLS_DISABLED) {
		tls_session = initialize_tls_session();
		src = (void *) &tls_session;
	}
	
	else
		src = (void *) &cfd;
	
	/* init repsonse */
	response.status = HTTP_STATUS_SERVICE;
	response.vmajor = 1;
	response.vminor = 0;
	
	headers = NULL;
	
	/* check number of clients */
	if (num_clients >= max_clients) {
		log_warn("Maximum number of connections reached");
		log_info("Rejecting client from %s:%d",
		         inet_ntop(AF_INET, &peer_addr.sin_addr, peer, INET_ADDRSTRLEN),
		         ntohs(peer_addr.sin_port));
		http_send_response(src, &response, headers, NULL, httpd_write);
		close(cfd);
		exit(EXIT_FAILURE);
	}
		
	log_info("New connection from %s, port %d",
	         inet_ntop(AF_INET, &peer_addr.sin_addr, peer, INET_ADDRSTRLEN),
	         ntohs(peer_addr.sin_port));
	
	headers = (http_header_t *) malloc(sizeof(http_header_t));
	INIT_LIST_HEAD(&(headers->list));
	
	if (http_get_request(src, &request, headers, &body, httpd_read) == -1)
		log_error_and_die("Could not get request: %s", strerror(errno));
	
	http_headers_free(headers);
	
	/* remove timeout */
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	
	/* handle request */
	headers = NULL;
	
	if (request.method != HTTP_METHOD_POST || strcmp(request.uri, "/RPC2") != 0)
		response.status = HTTP_STATUS_BADREQ;
	
	else if (method_call(xmlrpc_server, body, &rbody) == -1)
		response.status = HTTP_STATUS_BADREQ;
	
	else if (!rbody)
		response.status = HTTP_STATUS_INTERNAL;
	
	else {
		response.status = HTTP_STATUS_OK;
		
		headers = (http_header_t *) malloc(sizeof(http_header_t));
		INIT_LIST_HEAD(&(headers->list));
		
		tmp = (http_header_t *) malloc(sizeof(http_header_t));
		asprintf(&(tmp->key), "Content-Length");
		asprintf(&(tmp->val), "%d", strlen(rbody));
		
		list_add(&(tmp->list), &(headers->list));
	}
	
	/* send response */
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
	int i;
	
	switch (sig) {
	case SIGCHLD:
		num_clients--;
		wait(NULL);
		break;
	
	case SIGTERM:
		log_info("Caught SIGTERM -- shutting down");
		
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
		
		close(sfd);
		
		vxdb_close();
		
		if (tls_mode == TLS_ANON)
			gnutls_anon_free_server_credentials(anon);
		
		if (tls_mode == TLS_X509)
			gnutls_certificate_free_credentials(x509);
		
		gnutls_global_deinit();
		
		exit(EXIT_FAILURE);
	}
}

void server_main(void)
{
	int rc, port;
	char *host;
	socklen_t peerlen;
	struct sigaction act;
	char *ca, *crl, *cert, *key;
	
	log_init("server", 0);
	log_info("Loading configuration");
	
	max_clients = cfg_getint(cfg, "client-max");
	
	/* init TLS */
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
	
	/* setup listen socket */
	port = cfg_getint(cfg, "listen-port");
	host = cfg_getstr(cfg, "listen-host");
	
	if ((sfd = tcp_listen(host, port, 20)) == -1)
		log_error_and_die("cannot listen: %s", strerror(errno));
	
	/* handle these signals on our own */
	sigfillset(&act.sa_mask);
	
	act.sa_flags     = SA_SIGINFO;
	act.sa_sigaction = server_signal_handler;
	
	sigaction(SIGCHLD, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	
	/* setup xmlrpc server */
	if ((xmlrpc_server = XMLRPC_ServerCreate()) == NULL)
		log_error_and_die("Cannot create XMLRPC instance");
	
	method_registry_init(xmlrpc_server);
	
	/* wait and create a new child for each connection */
	log_info("Accepting incoming connections on %s:%d", host, port);
	
	while (1) {
		peerlen = sizeof(struct sockaddr_in);
		cfd = accept(sfd, (struct sockaddr *) &peer_addr, &peerlen);
		
		if (cfd == -1) {
			if (errno != EINTR)
				log_warn("accept: %s", strerror(errno));
			
			continue;
		}
		
		switch (fork()) {
		case -1:
			log_warn("fork: %s", strerror(errno));
			break;
		
		case 0:
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
