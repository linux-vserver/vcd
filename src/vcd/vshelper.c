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
#include <fcntl.h>
#include <vserver.h>
#include <gnutls/gnutls.h>

#include "confuse.h"
#include "lucid.h"
#include "xmlrpc.h"

#include "log.h"

static const char *rcsid = "$Id: vdbm.c 160 2006-04-08 13:31:34Z hollow $";

typedef enum {
	TLS_DISABLED = 0,
	TLS_ANON     = 1,
	TLS_X509     = 2,
} tls_modes_t;

static tls_modes_t tls_mode = TLS_DISABLED;
static gnutls_anon_client_credentials_t anon;
static gnutls_certificate_credentials_t x509;
static gnutls_session_t tls_session;

static int cfd;

static char *action;
static xid_t xid;
static char *name;

static cfg_opt_t CFG_OPTS[] = {
	/* network configuration */
	CFG_STR("server-host", "127.0.0.1", CFGF_NONE),
	CFG_INT("server-port", 13386,       CFGF_NONE),
	CFG_STR("server-user", "vshelper",  CFGF_NONE),
	CFG_STR("server-pass", NULL,        CFGF_NONE),
	
	/* logging */
	CFG_STR("log-dir",   NULL, CFGF_NONE),
	CFG_INT("log-level", 3,    CFGF_NONE),
	
	/* SSL/TLS */
	CFG_INT("tls-mode",       0,    CFGF_NONE),
	CFG_STR("tls-ca-crt",     NULL, CFGF_NONE),
	CFG_END()
};

cfg_t *cfg;

typedef int (*helper_t)(void);

static int vshelper_restart(void);
static int vshelper_halt(void);
static int vshelper_poweroff(void);
static int vshelper_ignore(void);

typedef struct {
	char *action;
	helper_t func;
} action_t;

static action_t ACTIONS[] = {
	{ "restart",  vshelper_restart },
	{ "halt",     vshelper_halt },
	{ "poweroff", vshelper_halt },
	{ "swsusp",   vshelper_ignore },
	{ "startup",  vshelper_ignore },
	{ "shutdown", vshelper_ignore },
	{ "netup",    vshelper_ignore },
	{ "netdown",  vshelper_ignore },
	{ NULL, NULL }
};

static inline
void usage(int rc)
{
	printf("Usage: vshelper <action> <xid>\n"
	       "\n"
	       "Valid actions:\n"
	       " - restart, halt, poweroff, swsusp\n"
	       " - startup, shutdown\n"
	       " - netup, netdown\n");
	exit(rc);
}

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
	}
	
	gnutls_transport_set_ptr(session, (gnutls_transport_ptr_t) cfd);
	
	if ((rc = gnutls_handshake(session)) < 0) {
		gnutls_deinit(session);
		log_error_and_die("Handshake has failed: %s", gnutls_strerror(rc));
	}
	
	return session;
}

static
int fault(XMLRPC_VALUE value)
{
	char *fault_string = XMLRPC_VectorGetStringWithID(value, "faultString");
	int   fault_code   = XMLRPC_VectorGetIntWithID(value, "faultCode");
	
	if (fault_string || fault_code != 0)
		return 1;
	
	return 0;
}

static
XMLRPC_VALUE call(char *method, XMLRPC_VALUE params)
{
	int rc;
	http_request_t http_request;
	http_response_t http_response;
	http_header_t *http_headers, *tmp;
	void *server_dst;
	
	int port   = cfg_getint(cfg, "server-port");
	char *host = cfg_getstr(cfg, "server-host");
	
	if ((cfd = tcp_connect(host, port)) == -1)
		log_error_and_die("cannot connect: %s", strerror(errno));
	
	if (tls_mode != TLS_DISABLED) {
		tls_session = initialize_tls_session();
		server_dst = (void *) &tls_session;
	}
	
	else
		server_dst = (void *) &cfd;
	
	XMLRPC_REQUEST request = XMLRPC_RequestNew();
	XMLRPC_RequestSetMethodName(request, method);
	XMLRPC_RequestSetRequestType(request, xmlrpc_request_call);
	
	XMLRPC_VALUE data = XMLRPC_CreateVector(NULL, xmlrpc_vector_array);
	XMLRPC_RequestSetData(request, data);
	
	char *user = cfg_getstr(cfg, "server-user");
	char *pass = cfg_getstr(cfg, "server-pass");
	
	XMLRPC_VALUE auth = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	XMLRPC_AddValueToVector(auth, XMLRPC_CreateValueString("username", user, 0));
	XMLRPC_AddValueToVector(auth, XMLRPC_CreateValueString("password", pass, 0));
	XMLRPC_AddValueToVector(data, auth);
	
	XMLRPC_AddValueToVector(data, params);
	
	char *buf = XMLRPC_REQUEST_ToXML(request, 0);
	
	XMLRPC_RequestFree(request, 1);
	
	if (!buf)
		return errno = EIO, NULL;
	
	http_request.method = HTTP_METHOD_POST;
	http_request.vmajor = 1;
	http_request.vminor = 0;
	
	strncpy(http_request.uri, "/RPC2", HTTP_MAX_URI - 1);
	
	http_headers = (http_header_t *) malloc(sizeof(http_header_t));
	INIT_LIST_HEAD(&(http_headers->list));
	
	tmp = (http_header_t *) malloc(sizeof(http_header_t));
	asprintf(&(tmp->key), "Content-Length");
	asprintf(&(tmp->val), "%d", strlen(buf));
	
	list_add(&(tmp->list), &(http_headers->list));
	
	rc = http_send_request(server_dst, &http_request,
	                       http_headers, buf, httpd_write);
	
	http_headers_free(http_headers);
	free(buf);
	
	if (rc == -1)
		return NULL;
	
	http_headers = (http_header_t *) malloc(sizeof(http_header_t));
	INIT_LIST_HEAD(&(http_headers->list));
	
	rc = http_get_response(server_dst, &http_response,
	                       http_headers, &buf, httpd_read);
	
	http_headers_free(http_headers);
	
	XMLRPC_REQUEST response = XMLRPC_REQUEST_FromXML(buf, strlen(buf), NULL);
	free(buf);
	
	data = XMLRPC_RequestGetData(response);
	
	if (tls_mode != TLS_DISABLED)
		gnutls_bye(tls_session, GNUTLS_SHUT_RDWR);
	
	close(cfd);
	
	if (tls_mode != TLS_DISABLED)
		gnutls_deinit(tls_session);
	
	return data;
}

static
int vshelper_restart(void)
{
	XMLRPC_VALUE params = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	XMLRPC_AddValueToVector(params, XMLRPC_CreateValueInt("wait", 0));
	XMLRPC_AddValueToVector(params, XMLRPC_CreateValueInt("reboot", 1));
	XMLRPC_AddValueToVector(params, XMLRPC_CreateValueString("name", name, 0));
	
	XMLRPC_VALUE response = call("vx.killer", params);
	
	if (!response || fault(response)) {
		log_warn("could not start vx.killer");
		return -1;
	}
	
	return 0;
}

static
int vshelper_halt(void)
{
	XMLRPC_VALUE params = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	XMLRPC_AddValueToVector(params, XMLRPC_CreateValueInt("wait", 0));
	XMLRPC_AddValueToVector(params, XMLRPC_CreateValueInt("reboot", 0));
	XMLRPC_AddValueToVector(params, XMLRPC_CreateValueString("name", name, 0));
	
	XMLRPC_VALUE response = call("vx.killer", params);
	
	if (!response || fault(response)) {
		log_warn("could not start vx.killer");
		return -1;
	}
	
	return 0;
}

static
int vshelper_ignore(void)
{
	return 0;
}

static
void exit_cleanup_handler(void)
{
	/* shutdown connection & exit */
	if (tls_mode == TLS_ANON)
		gnutls_anon_free_client_credentials(anon);
	
	if (tls_mode == TLS_X509)
		gnutls_certificate_free_credentials(x509);
	
	if (tls_mode != TLS_DISABLED)
		gnutls_global_deinit();
}

int main(int argc, char *argv[])
{
	int i, rc;
	
	if (argc != 3)
		usage(EXIT_FAILURE);
	
	action = argv[1];
	xid    = atoi(argv[2]);
	
	/* check command line */
	if (xid < 2)
		dprintf(STDERR_FILENO, "xid must be >= 2");
	
	for (i = 0; ACTIONS[i].action; i++)
		if (strcmp(ACTIONS[i].action, action) == 0)
			goto loadcfg;
	
	dprintf(STDERR_FILENO, "invalid action: %s", action);
	exit(EXIT_FAILURE);
	
loadcfg:
	/* load configuration */
	cfg = cfg_init(CFG_OPTS, CFGF_NOCASE);
	
	switch (cfg_parse(cfg, "/etc/vshelper.conf")) { /* TODO: look in more places */
	case CFG_FILE_ERROR:
		perror("cfg_parse");
		exit(EXIT_FAILURE);
	
	case CFG_PARSE_ERROR:
		dprintf(STDERR_FILENO, "cfg_parse: Parse error");
		exit(EXIT_FAILURE);
	
	default:
		break;
	}
	
	/* start logging & debugging */
	if (log_init("vshelper", 1) == -1) {
		perror("log_init");
		exit(EXIT_FAILURE);
	}
	
	/* init TLS */
	tls_mode = cfg_getint(cfg, "tls-mode");
	
	gnutls_global_init();
	
	if (tls_mode == TLS_ANON) {
		gnutls_anon_allocate_client_credentials(&anon);
		log_info("TLS with anonymous authentication configured successfully");
	}
	
	else if (tls_mode == TLS_X509) {
		char *ca   = cfg_getstr(cfg, "tls-ca-crt");
		
		if ((rc = gnutls_certificate_allocate_credentials(&x509)) < 0)
			log_error_and_die("gnuttls_certificate_allocate_credentials: %s", gnutls_strerror(rc));
		
		if (ca && (rc = gnutls_certificate_set_x509_trust_file(x509, ca, GNUTLS_X509_FMT_PEM)) < 0)
			log_error_and_die("gnuttls_certificate_set_x509_trust_file: %s", gnutls_strerror(rc));
		
		log_info("TLS with X.509 authentication configured successfully");
	}
	
	atexit(exit_cleanup_handler);
	
	/* get name by xid */
	XMLRPC_VALUE params = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	XMLRPC_AddValueToVector(params, XMLRPC_CreateValueInt("xid", xid));
	
	XMLRPC_VALUE response = call("vxdb.name.get", params);
	
	XMLRPC_CleanupValue(params);
	
	if (!response || fault(response))
		log_error_and_die("could not get response: %s", strerror(errno));
	
	name = XMLRPC_VectorGetStringWithID(response, "name");
	
	if (!name)
		log_error_and_die("could not get name for xid %d", xid);
	
	/* handle action */
	if (ACTIONS[i].func() == -1)
		log_error_and_die("action '%s' failed: %s", action, strerror(errno));
	
	/* shutdown connection & exit */
	if (tls_mode == TLS_ANON)
		gnutls_anon_free_client_credentials(anon);
	
	if (tls_mode == TLS_X509)
		gnutls_certificate_free_credentials(x509);
	
	if (tls_mode != TLS_DISABLED)
		gnutls_global_deinit();
	
	return 0;
}
