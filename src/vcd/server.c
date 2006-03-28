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
#include <sys/time.h>

#include "httpd.h"
#include "xmlrpc.h"

#include "log.h"
#include "server.h"

#include "methods/methods.h"

static XMLRPC_SERVER xmlrpc_server;

static
void handle_rpc2(httpd *httpd_server)
{
	XMLRPC_REQUEST request, response;
	char *buf;
	
	size_t len = strlen(httpd_server->fullRequest);
	request = XMLRPC_REQUEST_FromXML(httpd_server->fullRequest, len, NULL);
	
	if (!request) {
		httpdEndRequest(httpd_server);
		return;
	}
	
	/* create a response struct */
	response = XMLRPC_RequestNew();
	XMLRPC_RequestSetRequestType(response, xmlrpc_request_response);
	
	/* call server method with client request and assign the response to our response struct */
	XMLRPC_RequestSetData(response, XMLRPC_ServerCallMethod(xmlrpc_server, request, NULL));
	
	/* be courteous. reply in same vocabulary/manner as the request. */
	XMLRPC_RequestSetOutputOptions(response, XMLRPC_RequestGetOutputOptions(request) );
	
	/* serialize server response as XML */
	buf = XMLRPC_REQUEST_ToXML(response, 0);
	
	if (buf) {
		httpdPrintf(httpd_server, buf);
		free(buf);
	}
	
	XMLRPC_RequestFree(request, 1);
	XMLRPC_RequestFree(response, 1);
	
	httpdEndRequest(httpd_server);
	
	return;
}

void server_main(char *ip, int port)
{
	httpd         *httpd_server;
	
	/* setup xmlrpc server */
	xmlrpc_server = XMLRPC_ServerCreate();
	
	register_methods(xmlrpc_server);
	
	/* Ensure that PIPE signals are either handled or ignored.
	** If a client connection breaks while the server is using
	** it then the application will be sent a SIGPIPE.  If we
	** don't handle it then it'll terminate the server. */
	signal(SIGPIPE, SIG_IGN);
	
	/* create httpd server */
	httpd_server = httpdCreate(ip, port);
	
	if (!httpd_server)
		LOGPERR("httpdCreate");
	
	/* setup XMLRPC handler */
	httpdAddCContent(httpd_server, "/", "RPC2", HTTP_FALSE, NULL, handle_rpc2);
	
	while(1) {
		if (httpdGetConnection(httpd_server, NULL) == -1) {
			LOGPWARN("httpdGetConnection");
			continue;
		}
		
		if(httpdReadRequest(httpd_server) < 0)
			httpdEndRequest(httpd_server);
		
		else {
			httpdProcessRequest(httpd_server);
			httpdEndRequest(httpd_server);
		}
	}
}
