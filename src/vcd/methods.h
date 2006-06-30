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
#define MESYS     500

extern m_err_t method_error_codes[];

int method_registry_init(xmlrpc_env *env, xmlrpc_registry *registry);

xmlrpc_value *method_init(xmlrpc_env *env, xmlrpc_value *p,
                          uint64_t caps, int ownercheck);

char *method_strerror(int errnum);

#define method_return_if_fault(ENV) do { \
	if (ENV->fault_occurred) return NULL; \
} while (0)

#define method_return_fault(ENV, ID) do { \
	xmlrpc_env_set_fault(ENV, ID, method_strerror(ID)); \
	return NULL; \
} while (0)

#define method_return_faultf(ENV, ID, FMT, ...) do { \
	xmlrpc_env_set_fault_formatted(ENV, ID, FMT, __VA_ARGS__); \
	return NULL; \
} while(0)

#define MPROTO(NAME) \
	xmlrpc_value *NAME(xmlrpc_env *env, xmlrpc_value *p, void *c)

/* vx */
/*
MPROTO(m_vx_create);
MPROTO(m_vx_killer);
MPROTO(m_vx_restart);
MPROTO(m_vx_start);
MPROTO(m_vx_stop);
*/

/* vxdb */
MPROTO(m_vxdb_dx_limit_get);
MPROTO(m_vxdb_dx_limit_remove);
MPROTO(m_vxdb_dx_limit_set);
MPROTO(m_vxdb_init_method_get);
MPROTO(m_vxdb_init_method_set);
MPROTO(m_vxdb_list);
MPROTO(m_vxdb_mount_get);
MPROTO(m_vxdb_mount_remove);
MPROTO(m_vxdb_mount_set);
MPROTO(m_vxdb_name_get);
MPROTO(m_vxdb_nx_addr_get);
MPROTO(m_vxdb_nx_addr_remove);
MPROTO(m_vxdb_nx_addr_set);
MPROTO(m_vxdb_owner_add);
MPROTO(m_vxdb_owner_get);
MPROTO(m_vxdb_owner_remove);
MPROTO(m_vxdb_remove);
MPROTO(m_vxdb_user_get);
MPROTO(m_vxdb_user_remove);
MPROTO(m_vxdb_user_set);
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
