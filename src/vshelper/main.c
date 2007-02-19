// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <confuse.h>
#include <vserver.h>
#include <syslog.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include <lucid/chroot.h>
#include <lucid/exec.h>
#include <lucid/log.h>
#include <lucid/open.h>
#include <lucid/printf.h>
#include <lucid/scanf.h>
#include <lucid/str.h>

#include "cfg.h"

static char *uri;
static const char *user;
static const char *pass;

static cfg_opt_t CFG_OPTS[] = {
	CFG_STR("host",    "localhost", CFGF_NONE),
	CFG_INT("port",    13386,       CFGF_NONE),
	CFG_STR("user",    "vshelper",  CFGF_NONE),
	CFG_STR("pass",    NULL,        CFGF_NONE),
	CFG_STR("logfile", NULL,        CFGF_NONE),
	CFG_END()
};

cfg_t *cfg;

typedef int (*helper_t)(xmlrpc_env *env, xid_t xid);

static int vshelper_restart (xmlrpc_env *env, xid_t xid);
static int vshelper_halt    (xmlrpc_env *env, xid_t xid);
static int vshelper_swsusp  (xmlrpc_env *env, xid_t xid);
static int vshelper_startup (xmlrpc_env *env, xid_t xid);
static int vshelper_shutdown(xmlrpc_env *env, xid_t xid);
static int vshelper_netup   (xmlrpc_env *env, xid_t xid);
static int vshelper_netdown (xmlrpc_env *env, xid_t xid);

typedef struct {
	char *name;
	helper_t func;
} action_t;

static action_t ACTIONS[] = {
	{ "restart",  vshelper_restart },
	{ "halt",     vshelper_halt },
	{ "poweroff", vshelper_halt },
	{ "swsusp",   vshelper_swsusp },
	{ "startup",  vshelper_startup },
	{ "shutdown", vshelper_shutdown },
	{ "netup",    vshelper_netup },
	{ "netdown",  vshelper_netdown },
	{ NULL, NULL }
};

#define SIGNATURE(S) "({s:s,s:s}" S ")", "username", user, "password", pass

#define log_and_return_if_fault(ENV) do { \
	if (ENV->fault_occurred) { \
		log_error("xmlrpc error: %s", ENV->fault_string); \
		return EXIT_FAILURE; \
	} \
} while (0)

static
int vshelper_restart(xmlrpc_env *env, xid_t xid)
{
	nx_flags_t nflags = {
		.flags = 0,
		.mask = NXF_PERSISTENT,
	};

	vx_flags_t vflags = {
		.flags = VXF_REBOOT_KILL,
		.mask  = VXF_REBOOT_KILL|VXF_PERSISTENT,
	};

	log_info("context %d has commited suicide with rebirth request", xid);

	xmlrpc_client_call(env, uri, "helper.restart",
			SIGNATURE("{s:i}"),
			"xid", xid);
	log_and_return_if_fault(env);

	log_info("context %d has been scheduled for rebirth", xid);

	if (nx_flags_set(xid, &nflags) == -1)
		log_perror("nx_flags_set(%d)", xid);

	else if (vx_flags_set(xid, &vflags) == -1)
		log_perror("vx_flags_set(%d)", xid);

	else
		return EXIT_SUCCESS;

	return EXIT_FAILURE;
}

static
int vshelper_halt(xmlrpc_env *env, xid_t xid)
{
	nx_flags_t nflags = {
		.flags = 0,
		.mask = NXF_PERSISTENT,
	};

	vx_flags_t vflags = {
		.flags = VXF_REBOOT_KILL,
		.mask  = VXF_REBOOT_KILL|VXF_PERSISTENT,
	};

	log_info("context %d has commited suicide", xid);

	if (nx_flags_set(xid, &nflags) == -1)
		log_perror("nx_flags_set(%d)", xid);

	else if (vx_flags_set(xid, &vflags) == -1)
		log_perror("vx_flags_set(%d)", xid);

	else
		return EXIT_SUCCESS;

	return EXIT_FAILURE;
}

static
int vshelper_swsusp(xmlrpc_env *env, xid_t xid)
{
	log_info("context %d has requested to be suspended - fool!", xid);

	return EXIT_FAILURE;
}

static
int vshelper_startup(xmlrpc_env *env, xid_t xid)
{
	xmlrpc_value *response;
	char *vdir, *init;
	int i;

	vx_flags_t migrate_flags = {
		.flags = VXM_SET_INIT|VXM_SET_REAPER,
	};

	log_info("context %d emerges from the darkness", xid);

	response = xmlrpc_client_call(env, uri, "helper.startup",
			SIGNATURE("{s:i}"),
			"xid", xid);
	log_and_return_if_fault(env);

	xmlrpc_decompose_value(env, response,
			"{s:s,s:s,*}",
			"vdir", &vdir,
			"init", &init);
	log_and_return_if_fault(env);

	switch (fork()) {
	case -1:
		log_perror("fork");
		break;

	case 0:
		if (ns_enter(xid, 0) == -1)
			log_perror("ns_enter(%d)", xid);

		else if (chroot_secure_chdir(vdir, "/") == -1)
			log_perror("chroot_secure_chdir(%s)", vdir);

		else if (chroot(".") == -1)
			log_perror("chroot");

		else if (nx_migrate(xid) == -1)
			log_perror("nx_migrate(%d)", xid);

		for (i = 0; i < 256; i++)
			close(i);

		if (vx_migrate(xid, &migrate_flags) == -1)
			exit(errno);

		clearenv();

		exec_replace(init);
		exit(EXIT_FAILURE);

	default:
		log_info("context %d is up - now starting init", xid);
		break;
	}

	int n = 0;
	vx_stat_t stat;
	nx_flags_t nflags = { .flags = 0, .mask = VXF_PERSISTENT };
	vx_flags_t vflags = { .flags = 0, .mask = NXF_PERSISTENT };

	while (n++ < 15) {
		sleep(1);

		if (vx_stat(xid, &stat) == -1)
			log_perror("vx_stat(%d)", xid);

		else if (stat.tasks > 1) {
			if (nx_flags_set(xid, &nflags) == -1)
				log_perror("nx_flags_set(%d)", xid);

			if (vx_flags_set(xid, &vflags) == -1)
				log_perror("vx_flags_set(%d)", xid);

			return EXIT_SUCCESS;
		}

		log_error("task count for context %d still zero after %d %s",
				xid, n, n > 1 ? "seconds" : "second");
	}

	log_error("context %d failed to start init - shutting down", xid);

	if (nx_flags_set(xid, &nflags) == -1)
		log_perror("nx_flags_set(%d)", xid);

	if (vx_flags_set(xid, &vflags) == -1)
		log_perror("vx_flags_set(%d)", xid);

	return EXIT_FAILURE;
}

static
int vshelper_shutdown(xmlrpc_env *env, xid_t xid)
{
	xmlrpc_value *response;

	log_info("context %d has fallen into oblivion - checking rebirth schedule",
			xid);

	response = xmlrpc_client_call(env, uri, "helper.shutdown",
			SIGNATURE("{s:i}"),
			"xid", xid);
	log_and_return_if_fault(env);

	return EXIT_SUCCESS;
}

static
int vshelper_netup(xmlrpc_env *env, xid_t xid)
{
	log_info("network context %d emerges from the darkness", xid);

	xmlrpc_client_call(env, uri, "helper.netup",
			SIGNATURE("{s:i}"),
			"xid", xid);
	log_and_return_if_fault(env);

	log_info("network context %d is up", xid);

	return EXIT_SUCCESS;
}

static
int vshelper_netdown(xmlrpc_env *env, xid_t xid)
{
	log_info("network context %d has fallen into oblivion", xid);

	return EXIT_SUCCESS;
}

static
int do_vshelper(xid_t xid, action_t *action)
{
	char *host;
	int port, ret = EXIT_FAILURE;
	xmlrpc_env env;

	host = cfg_getstr(cfg, "host");
	port = cfg_getint(cfg, "port");
	user = cfg_getstr(cfg, "user");
	pass = cfg_getstr(cfg, "pass");

	asprintf(&uri, "http://%s:%d/RPC2", host, port);

	/* init xmlrpc environment */
	xmlrpc_env_init(&env);

	/* init xmlrpc client */
	xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, "vshelper",
			PACKAGE_VERSION, NULL, 0);

	if (env.fault_occurred)
		log_error("could not init xmlrpc client: %s", env.fault_string);

	else
		ret = action->func(&env, xid);

	xmlrpc_env_clean(&env);
	xmlrpc_client_cleanup();

	log_info("vshelper returns with %d", ret);

	return ret;
}

int main(int argc, char *argv[])
{
	int i, xid;
	const char *action, *logfile;

	/* load configuration */
	cfg = cfg_init(CFG_OPTS, CFGF_NOCASE);

	switch (cfg_parse(cfg, SYSCONFDIR "/vshelper.conf")) {
	case CFG_FILE_ERROR:
		exit(EXIT_FAILURE);

	case CFG_PARSE_ERROR:
		exit(EXIT_FAILURE);

	default:
		break;
	}

	atexit(cfg_atexit);

	/* start logging & debugging */
	log_options_t log_options = {
		.log_ident = argv[0],
		.log_dest = LOGD_SYSLOG,
		.log_opts = LOGO_PRIO|LOGO_TIME|LOGO_IDENT|LOGO_PID,
	};

	logfile = cfg_getstr(cfg, "logfile");

	if (!str_isempty(logfile)) {
		log_options.log_dest |= LOGD_FILE;
		log_options.log_fd = open_append(logfile);
	}

	log_init(&log_options);

	/* parse command line */
	if (argc < 2)
		log_error_and_die("no action specified");

	if (argc < 3)
		log_error_and_die("no xid specified");

	action = argv[1];
	sscanf(argv[2], "%d", &xid);

	if (xid < 2 || xid > 65535)
		log_error_and_die("invalid xid: %d", xid);

	for (i = 0; ACTIONS[i].name; i++)
		if (str_equal(ACTIONS[i].name, action))
			exit(do_vshelper(xid, &ACTIONS[i]));

	log_error("invalid action: %s", action);
	exit(EXIT_FAILURE);
}
