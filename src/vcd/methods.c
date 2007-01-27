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

#include <unistd.h>
#include <limits.h>

#include "auth.h"
#include "cfg.h"
#include "methods.h"

#include <lucid/log.h>
#include <lucid/misc.h>
#include <lucid/open.h>
#include <lucid/str.h>

m_err_t method_error_codes[] = {
	{ MEAUTH,    "Unauthorized" },
	{ MEPERM,    "Operation not permitted" },
	{ MEINVAL,   "Invalid request" },
	{ MEVXDB,    "Error in vxdb" },
	{ MECONF,    "Invalid configuration" },
	{ MESTOPPED, "Not running" },
	{ MERUNNING, "Already running" },
	{ MEEXIST,   "Conflict/Already exists" },
	{ MENOVPS,   "Not found" },
	{ MESYS,     "System call failed" },
	{ 0,         NULL },
};

xmlrpc_registry *registry;
void *METHOD_INTERNAL = (void *) 0xdeadbeef;

static int lockfd = -1;

#define MREGISTER(NAME,FUNC) do { \
	xmlrpc_registry_add_method(env, registry, NULL, NAME, &FUNC, NULL); \
} while (0)

int method_registry_init(xmlrpc_env *env)
{
	LOG_TRACEME

	/* helper */
	MREGISTER("helper.netup",    m_helper_netup);
	MREGISTER("helper.restart",  m_helper_restart);
	MREGISTER("helper.shutdown", m_helper_shutdown);
	MREGISTER("helper.startup",  m_helper_startup);

	/* vx */
	MREGISTER("vx.create",  m_vx_create);
	MREGISTER("vx.exec",    m_vx_exec);
	MREGISTER("vx.kill",    m_vx_kill);
	MREGISTER("vx.reboot",  m_vx_reboot);
	MREGISTER("vx.remove",  m_vx_remove);
	MREGISTER("vx.rename",  m_vx_rename);
	MREGISTER("vx.start",   m_vx_start);
	MREGISTER("vx.status",  m_vx_status);
	MREGISTER("vx.stop",    m_vx_stop);

	/* vxdb */
	MREGISTER("vxdb.dx.limit.get",        m_vxdb_dx_limit_get);
	MREGISTER("vxdb.dx.limit.remove",     m_vxdb_dx_limit_remove);
	MREGISTER("vxdb.dx.limit.set",        m_vxdb_dx_limit_set);
	MREGISTER("vxdb.init.get",            m_vxdb_init_get);
	MREGISTER("vxdb.init.set",            m_vxdb_init_set);
	MREGISTER("vxdb.list",                m_vxdb_list);
	MREGISTER("vxdb.mount.get",           m_vxdb_mount_get);
	MREGISTER("vxdb.mount.remove",        m_vxdb_mount_remove);
	MREGISTER("vxdb.mount.set",           m_vxdb_mount_set);
	MREGISTER("vxdb.name.get",            m_vxdb_name_get);
	MREGISTER("vxdb.nx.addr.get",         m_vxdb_nx_addr_get);
	MREGISTER("vxdb.nx.addr.remove",      m_vxdb_nx_addr_remove);
	MREGISTER("vxdb.nx.addr.set",         m_vxdb_nx_addr_set);
	MREGISTER("vxdb.nx.broadcast.get",    m_vxdb_nx_broadcast_get);
	MREGISTER("vxdb.nx.broadcast.remove", m_vxdb_nx_broadcast_remove);
	MREGISTER("vxdb.nx.broadcast.set",    m_vxdb_nx_broadcast_set);
	MREGISTER("vxdb.owner.add",           m_vxdb_owner_add);
	MREGISTER("vxdb.owner.get",           m_vxdb_owner_get);
	MREGISTER("vxdb.owner.remove",        m_vxdb_owner_remove);
	MREGISTER("vxdb.user.caps.add",       m_vxdb_user_caps_add);
	MREGISTER("vxdb.user.caps.get",       m_vxdb_user_caps_get);
	MREGISTER("vxdb.user.caps.remove",    m_vxdb_user_caps_remove);
	MREGISTER("vxdb.user.get",            m_vxdb_user_get);
	MREGISTER("vxdb.user.remove",         m_vxdb_user_remove);
	MREGISTER("vxdb.user.set",            m_vxdb_user_set);
	MREGISTER("vxdb.vdir.get",            m_vxdb_vdir_get);
	MREGISTER("vxdb.vx.bcaps.add",        m_vxdb_vx_bcaps_add);
	MREGISTER("vxdb.vx.bcaps.get",        m_vxdb_vx_bcaps_get);
	MREGISTER("vxdb.vx.bcaps.remove",     m_vxdb_vx_bcaps_remove);
	MREGISTER("vxdb.vx.ccaps.add",        m_vxdb_vx_ccaps_add);
	MREGISTER("vxdb.vx.ccaps.get",        m_vxdb_vx_ccaps_get);
	MREGISTER("vxdb.vx.ccaps.remove",     m_vxdb_vx_ccaps_remove);
	MREGISTER("vxdb.vx.flags.add",        m_vxdb_vx_flags_add);
	MREGISTER("vxdb.vx.flags.get",        m_vxdb_vx_flags_get);
	MREGISTER("vxdb.vx.flags.remove",     m_vxdb_vx_flags_remove);
	MREGISTER("vxdb.vx.limit.get",        m_vxdb_vx_limit_get);
	MREGISTER("vxdb.vx.limit.remove",     m_vxdb_vx_limit_remove);
	MREGISTER("vxdb.vx.limit.set",        m_vxdb_vx_limit_set);
	MREGISTER("vxdb.vx.sched.get",        m_vxdb_vx_sched_get);
	MREGISTER("vxdb.vx.sched.remove",     m_vxdb_vx_sched_remove);
	MREGISTER("vxdb.vx.sched.set",        m_vxdb_vx_sched_set);
	MREGISTER("vxdb.vx.uname.get",        m_vxdb_vx_uname_get);
	MREGISTER("vxdb.vx.uname.remove",     m_vxdb_vx_uname_remove);
	MREGISTER("vxdb.vx.uname.set",        m_vxdb_vx_uname_set);
	MREGISTER("vxdb.xid.get",             m_vxdb_xid_get);

	return 0;
}

#undef MREGISTER

void method_registry_atexit(void)
{
	LOG_TRACEME
	xmlrpc_registry_free(registry);
}

static
void method_lock(xmlrpc_env *env, const char *name)
{
	LOG_TRACEME

	if(lockfd != -1) {
		method_set_fault(env, MEBUSY);
		return;
	}

	char lockfile[PATH_MAX], *datadir = cfg_getstr(cfg, "datadir");

	snprintf(lockfile, PATH_MAX, "%s/lock/%s", datadir, name);
	mkdirnamep(lockfile, 0755);

	lockfd = open_trunc(lockfile);

	if (lockf(lockfd, F_TEST, 0) == -1)
		method_set_fault(env, MEBUSY);
	else
		lockf(lockfd, F_LOCK, 0);
}

static
void method_check_flags(xmlrpc_env *env, xmlrpc_value *params, void *c,
                        char *user, uint64_t flags)
{
	LOG_TRACEME

	char *name;

	xmlrpc_decompose_value(env, params, "{s:s,*}", "name", &name);

	if (!env->fault_occurred) {
		if (str_isempty(name))
			method_set_fault(env, MEINVAL);

		else if (user && (flags & M_OWNER) && !auth_isowner(user, name))
			method_set_fault(env, MENOVPS);

		else if (c && (flags & M_LOCK))
			method_lock(env, name);
	}
}

xmlrpc_value *method_init(xmlrpc_env *env, xmlrpc_value *p, void *c,
                          uint64_t caps, uint64_t flags)
{
	LOG_TRACEME

	char *user = NULL, *pass;
	xmlrpc_value *params = NULL;

	if (!c) {
		xmlrpc_decompose_value(env, p,
			"({s:s,s:s,*}V)",
			"username", &user,
			"password", &pass,
			&params);
		method_return_if_fault(env);

		if (!auth_isvalid(user, pass))
			method_set_fault(env, MEAUTH);

		else if (!auth_capable(user, caps))
			method_set_fault(env, MEPERM);

		else if (flags)
			method_check_flags(env, params, c, user, flags);
	}

	else if (flags)
		method_check_flags(env, p, c, NULL, flags);

	if (env->fault_occurred)
		return NULL;
	else if (params)
		return params;
	else
		return p;
}

void method_empty_params(int num, ...)
{
	LOG_TRACEME

	int i;
	va_list ap;
	char **ptr;

	va_start(ap, num);

	for (i = 0; i < num; i++) {
		ptr = va_arg(ap, char **);

		if (!*ptr)
			continue;

		if (str_isempty(*ptr)) {
			*ptr = NULL;
		}
	}

	va_end(ap);
}

char *method_strerror(int id)
{
	LOG_TRACEME

	int i;

	for (i = 0; method_error_codes[i].msg; i++)
		if (method_error_codes[i].id == id)
			return method_error_codes[i].msg;

	return NULL;
}
