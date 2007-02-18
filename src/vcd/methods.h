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

#ifndef _VCD_METHODS_H
#define _VCD_METHODS_H

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>

typedef struct {
	int id;
	char *msg;
} m_err_t;

#define MEAUTH    100
#define MEPERM    101
#define MEINVAL   102
#define MEVXDB    200
#define MECONF    201
#define MENOUSER  202
#define MESTOPPED 300
#define MERUNNING 301
#define MEEXIST   400
#define MENOVPS   401
#define MEBUSY    402
#define MESYS     500
#define MEEXEC    1000 /* this should always be the last error code */

#define M_OWNER 0x0001
#define M_LOCK  0x0002

extern m_err_t method_error_codes[];
extern xmlrpc_registry *registry;
extern void *METHOD_INTERNAL;

int method_registry_init(xmlrpc_env *env);
void method_registry_atexit(void);

xmlrpc_value *method_init(xmlrpc_env *env, xmlrpc_value *p, void *c,
                          uint64_t caps, uint64_t flags);

void method_empty_params(int num, ...);
char *method_strerror(int id);

#define method_cleanup_if_fault(ENV) do { \
	if (ENV->fault_occurred) goto cleanup; \
} while (0)

#define method_return_if_fault(ENV) do { \
	if (ENV->fault_occurred) return NULL; \
} while (0)

#define method_set_fault(ENV, ID) do { \
	xmlrpc_env_set_fault(ENV, ID, method_strerror(ID)); \
} while(0)

#define method_set_faultf(ENV, ID, FMT, ...) do { \
    xmlrpc_env_set_fault_formatted(ENV, ID, FMT, __VA_ARGS__); \
} while(0)

#define method_set_sys_fault(ENV, MSG) do { \
	xmlrpc_env_set_fault_formatted(ENV, MESYS, "%s(%d): %s: %s", \
			__FUNCTION__, __LINE__, MSG, strerror(errno)); \
} while (0)

#define method_set_sys_faultf(ENV, MSG, ...) do { \
	xmlrpc_env_set_fault_formatted(ENV, MESYS, "%s(%d): " MSG ": %s", \
			__FUNCTION__, __LINE__, __VA_ARGS__, strerror(errno)); \
} while (0)

#define method_set_vxdb_fault(ENV) do { \
	xmlrpc_env_set_fault_formatted(ENV, MEVXDB, "%s: error in vxdb: %s", \
			__FUNCTION__, vxdb_errmsg(vxdb)); \
} while (0)

#define method_return_fault(ENV, ID) do { \
	method_set_fault(ENV, ID); \
	return NULL; \
} while (0)

#define method_return_faultf(ENV, ID, FMT, ...) do { \
    method_set_faultf(ENV, ID, FMT, __VA_ARGS__); \
    return NULL; \
} while(0)

#define method_return_sys_fault(ENV, MSG) do { \
	method_set_sys_fault(ENV, MSG); \
	return NULL; \
} while (0)

#define method_return_sys_faultf(ENV, MSG, ...) do { \
	method_set_sys_faultf(ENV, MSG, __VA_ARGS__); \
	return NULL; \
} while (0)

#define method_return_vxdb_fault(ENV) do { \
	method_set_vxdb_fault(ENV); \
	return NULL; \
} while (0)

#define MPROTO(NAME) \
	xmlrpc_value *NAME(xmlrpc_env *env, xmlrpc_value *p, void *c)

/* helper */
MPROTO(m_helper_netup);
MPROTO(m_helper_restart);
MPROTO(m_helper_shutdown);
MPROTO(m_helper_startup);

/* vcd */
MPROTO(m_vcd_login);
MPROTO(m_vcd_status);
MPROTO(m_vcd_user_caps_add);
MPROTO(m_vcd_user_caps_get);
MPROTO(m_vcd_user_caps_remove);
MPROTO(m_vcd_user_get);
MPROTO(m_vcd_user_remove);
MPROTO(m_vcd_user_set);

/* vg */
MPROTO(m_vg_add);
MPROTO(m_vg_del);
MPROTO(m_vg_list);

/* vx */
MPROTO(m_vx_create);
MPROTO(m_vx_exec);
MPROTO(m_vx_kill);
MPROTO(m_vx_load);
MPROTO(m_vx_reboot);
MPROTO(m_vx_remove);
MPROTO(m_vx_rename);
MPROTO(m_vx_restart);
MPROTO(m_vx_start);
MPROTO(m_vx_status);
MPROTO(m_vx_stop);
MPROTO(m_vx_templates);

/* vxdb */
MPROTO(m_vxdb_dx_limit_get);
MPROTO(m_vxdb_dx_limit_remove);
MPROTO(m_vxdb_dx_limit_set);
MPROTO(m_vxdb_init_get);
MPROTO(m_vxdb_init_set);
MPROTO(m_vxdb_list);
MPROTO(m_vxdb_mount_get);
MPROTO(m_vxdb_mount_remove);
MPROTO(m_vxdb_mount_set);
MPROTO(m_vxdb_name_get);
MPROTO(m_vxdb_nx_addr_get);
MPROTO(m_vxdb_nx_addr_remove);
MPROTO(m_vxdb_nx_addr_set);
MPROTO(m_vxdb_nx_broadcast_get);
MPROTO(m_vxdb_nx_broadcast_remove);
MPROTO(m_vxdb_nx_broadcast_set);
MPROTO(m_vxdb_owner_add);
MPROTO(m_vxdb_owner_get);
MPROTO(m_vxdb_owner_remove);
MPROTO(m_vxdb_remove);
MPROTO(m_vxdb_vdir_get);
MPROTO(m_vxdb_vx_bcaps_add);
MPROTO(m_vxdb_vx_bcaps_get);
MPROTO(m_vxdb_vx_bcaps_remove);
MPROTO(m_vxdb_vx_ccaps_add);
MPROTO(m_vxdb_vx_ccaps_get);
MPROTO(m_vxdb_vx_ccaps_remove);
MPROTO(m_vxdb_vx_flags_add);
MPROTO(m_vxdb_vx_flags_get);
MPROTO(m_vxdb_vx_flags_remove);
MPROTO(m_vxdb_vx_limit_get);
MPROTO(m_vxdb_vx_limit_remove);
MPROTO(m_vxdb_vx_limit_set);
MPROTO(m_vxdb_vx_sched_get);
MPROTO(m_vxdb_vx_sched_remove);
MPROTO(m_vxdb_vx_sched_set);
MPROTO(m_vxdb_vx_uname_get);
MPROTO(m_vxdb_vx_uname_remove);
MPROTO(m_vxdb_vx_uname_set);
MPROTO(m_vxdb_xid_get);

#undef MPROTO

#endif
