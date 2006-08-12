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
#include <confuse.h>
#include <vserver.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

static char *uri;

static const char *name;
static const char *user;
static const char *pass;

static cfg_opt_t CFG_OPTS[] = {
	CFG_STR("host", "localhost", CFGF_NONE),
	CFG_INT("port", 13386,       CFGF_NONE),
	CFG_STR("user", "vshelper",  CFGF_NONE),
	CFG_STR("pass", NULL,        CFGF_NONE),
	CFG_END()
};

cfg_t *cfg;

typedef void (*helper_t)(xmlrpc_env *env);

static void vshelper_restart(xmlrpc_env *env);
static void vshelper_halt(xmlrpc_env *env);
static void vshelper_ignore(xmlrpc_env *env);

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

#define SIGNATURE(S) "({s:s,s:s}" S ")", "username", user, "password", pass

static
void vshelper_restart(xmlrpc_env *env)
{
	xmlrpc_client_call(env, uri, "vx.killer",
	                   SIGNATURE("{s:s,s:i,s:i}"),
	                   "name", name,
	                   "wait", 0,
	                   "reboot", 1);
}

static
void vshelper_halt(xmlrpc_env *env)
{
	xmlrpc_client_call(env, uri, "vx.killer",
	                   SIGNATURE("{s:s,s:i,s:i}"),
	                   "name", name,
	                   "wait", 0,
	                   "reboot", 0);
}

static
void vshelper_ignore(xmlrpc_env *env)
{
	return;
}

#define die_if_fault(ENV) do { \
	if (ENV.fault_occurred) { \
		dprintf(STDERR_FILENO, "%s (%d)\n", \
		        ENV.fault_string, ENV.fault_code); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

int main(int argc, char *argv[])
{
	xmlrpc_env env;
	xmlrpc_value *result;
	int i, xid, port;
	char *action, *host;
	
	if (argc != 3)
		usage(EXIT_FAILURE);
	
	action = argv[1];
	xid    = atoi(argv[2]);
	
	/* check command line */
	if (xid < 2 || xid > 65535)
		dprintf(STDERR_FILENO, "xid must be >= 2");
	
	for (i = 0; ACTIONS[i].action; i++)
		if (strcmp(ACTIONS[i].action, action) == 0)
			goto loadcfg;
	
	dprintf(STDERR_FILENO, "invalid action: %s", action);
	exit(EXIT_FAILURE);
	
loadcfg:
	/* load configuration */
	cfg = cfg_init(CFG_OPTS, CFGF_NOCASE);
	
	switch (cfg_parse(cfg, SYSCONFDIR "/vshelper.conf")) {
	case CFG_FILE_ERROR:
		perror("cfg_parse");
		exit(EXIT_FAILURE);
	
	case CFG_PARSE_ERROR:
		dprintf(STDERR_FILENO, "cfg_parse: Parse error\n");
		exit(EXIT_FAILURE);
	
	default:
		break;
	}
	
	host = cfg_getstr(cfg, "host");
	port = cfg_getint(cfg, "port");
	user = cfg_getstr(cfg, "user");
	pass = cfg_getstr(cfg, "pass");
	
	asprintf(&uri, "http://%s:%d/RPC2", host, port);
	
	/* init xmlrpc environment */
	xmlrpc_env_init(&env);
	
	/* init xmlrpc client */
	xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, "vshelper", PACKAGE_VERSION, NULL, 0);
	die_if_fault(env);
	
	/* get name by xid */
	result = xmlrpc_client_call(&env, uri, "vxdb.name.get",
	                            SIGNATURE("{s:i}"),
	                            "xid", xid);
	die_if_fault(env);
	
	xmlrpc_read_string(&env, result, &name);
	die_if_fault(env);
	
	/* handle action */
	ACTIONS[i].func(&env);
	die_if_fault(env);
	
	xmlrpc_DECREF(result);
	
	xmlrpc_env_clean(&env);
	xmlrpc_client_cleanup();
	
	free(uri);
	
	exit(EXIT_SUCCESS);
}
