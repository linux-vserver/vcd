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

#include "xmlrpc.h"

typedef struct {
	int id;
	char *msg;
} m_err_t;

#define MEAUTH    100
#define MEPERM    101
#define MEREQ     102
#define MEVXDB    200
#define MECONF    201
#define MESTOPPED 300
#define MERUNNING 301
#define MEEXIST   400
#define MENOENT   401
#define MESYS     500

extern m_err_t method_error_codes[];

void         method_registry_init(XMLRPC_SERVER s);

XMLRPC_VALUE method_get_params   (XMLRPC_REQUEST r);

char        *method_strerror     (int id);
XMLRPC_VALUE method_error        (int id);

#define MPROTO(NAME) \
	XMLRPC_VALUE NAME (XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)

/* vxdb */
MPROTO(m_vx_killer);
MPROTO(m_vx_restart);
MPROTO(m_vx_start);
MPROTO(m_vx_stop);
MPROTO(m_vxdb_create);
MPROTO(m_vxdb_dx_limit_get);
MPROTO(m_vxdb_dx_limit_remove);
MPROTO(m_vxdb_dx_limit_set);
MPROTO(m_vxdb_init_method_get);
MPROTO(m_vxdb_init_method_remove);
MPROTO(m_vxdb_init_method_set);
MPROTO(m_vxdb_init_mount_add);
MPROTO(m_vxdb_init_mount_get);
MPROTO(m_vxdb_init_mount_remove);
MPROTO(m_vxdb_nx_addr_add);
MPROTO(m_vxdb_nx_addr_get);
MPROTO(m_vxdb_nx_addr_remove);
MPROTO(m_vxdb_owner_add);
MPROTO(m_vxdb_owner_get);
MPROTO(m_vxdb_owner_remove);
MPROTO(m_vxdb_remove);
MPROTO(m_vxdb_user_add);
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
MPROTO(m_vxdb_vx_pflags_add);
MPROTO(m_vxdb_vx_pflags_get);
MPROTO(m_vxdb_vx_pflags_remove);
MPROTO(m_vxdb_vx_sched_get);
MPROTO(m_vxdb_vx_sched_remove);
MPROTO(m_vxdb_vx_sched_set);
MPROTO(m_vxdb_vx_uname_get);
MPROTO(m_vxdb_vx_uname_remove);
MPROTO(m_vxdb_vx_uname_set);

#undef MPROTO

#endif
