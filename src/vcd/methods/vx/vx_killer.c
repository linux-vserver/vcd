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
#include <string.h>
#include <signal.h>

#include "xmlrpc.h"

#include "auth.h"
#include "log.h"
#include "methods.h"
#include "vxdb.h"

/* killer process:
   1) fork to background
   2) wait for context to die using a timeout
   3) if timeout is reached kill all processes in context
   4) restart OR exit if all processes in context died
*/

static XMLRPC_SERVER _server;
static XMLRPC_REQUEST _request;

static char *name = NULL;
static xid_t xid = 0;
static int reboot = 0;

static
void handle_death(void)
{
	log_debug("vx killer success for '%s'", name);
	
	if (reboot == 0)
		exit(EXIT_SUCCESS);
	
	signal(SIGCHLD, SIG_DFL);
	
	log_debug("vx killer restarting '%s'", name);
	XMLRPC_VALUE response = m_vx_start(_server, _request, NULL);
	
	char *fault_string = XMLRPC_VectorGetStringWithID(response, "faultString");
	int   fault_code   = XMLRPC_VectorGetIntWithID(response, "faultCode");
	
	if (fault_string || fault_code != 0) {
		log_warn("vx killer restart failed: %s (%d)", fault_string, fault_code);
		exit(EXIT_FAILURE);
	}
	
	exit(EXIT_SUCCESS);
}

static
void wait_timeout_handler(int sig)
{
	int i;
	
	struct vx_kill_opts kill_opts = {
		.pid = 0,
		.sig = 9,
	};
	
	log_debug("vx killer reached timeout before death", name);
	
	signal(SIGALRM, SIG_DFL);
	
	for (i = 0; i < 5; i++) {
		log_debug("vx killer terminates all processes (%d)", i);
		vx_kill(xid, &kill_opts);
		sleep(1);
		
		if (vx_get_info(xid, NULL) == -1 && errno == ESRCH)
			break;
	}
	
	handle_death();
}

static
void wait_death(int timeout)
{
	signal(SIGALRM, wait_timeout_handler);
	alarm(timeout);
	log_debug("vx killer now waiting %d seconds for context death", timeout);
	
	if (vx_wait(xid, NULL) == -1 && errno != ESRCH)
		log_debug("vx killer could not wait: %s", strerror(errno));
	
	log_debug("vx killer didn't reach timeout before death");
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	
	handle_death();
}

/* vx.killer(string name, int reboot) */
XMLRPC_VALUE m_vx_killer(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	XMLRPC_VALUE response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	if (!auth_isadmin(r) && !auth_isowner(r))
		return method_error(MEPERM);
	
	_server = s;
	_request = r;
	
	name   = XMLRPC_VectorGetStringWithID(params, "name");
	reboot = XMLRPC_VectorGetIntWithID(params, "reboot");
	
	if (!name)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	if (vx_get_info(xid, NULL) == -1 && errno == ESRCH)
		return method_error(MESTOPPED);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT timeout FROM init_method WHERE xid = %d",
		xid);
	
	if (!dbr)
		return method_error(MEVXDB);
	
	int timeout = 0;
	
	if (dbi_result_get_numrows(dbr) > 0) {
		dbi_result_first_row(dbr);
		timeout = dbi_result_get_int(dbr, "timeout");
	}
	
	if (timeout == 0)
		timeout = 10;
	
	signal(SIGCHLD, SIG_IGN);
	
	pid_t pid;
	
	switch((pid = fork())) {
	case -1:
		return method_error(MESYS);
		break;
	
	case 0:
		log_debug("vx killer activated for '%s'", name);
		wait_death(timeout);
		break;
	
	default:
		break;
	}
	
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("name", name, 0));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("pid", pid));
	
	return response;
}
