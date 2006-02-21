/***************************************************************************
 *   Copyright 2005-2006 by the vserver-utils team                         *
 *   See AUTHORS for details                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _LIBVC_VC_H
#define _LIBVC_VC_H

#include <stdarg.h>
#include <vserver.h>
#include <lucid/flist.h>
#include <lucid/printf.h>

/* cfg.c */
enum {
	VC_CFG_BOOL_T = 1,
	VC_CFG_INT_T,
	VC_CFG_STR_T,
	VC_CFG_LIST_T,
};

typedef struct {
	char *key;
	int  type;
	char *file;
} vc_cfg_node_t;

extern vc_cfg_node_t vc_cfg_map[];

int vc_cfg_get_type(char *key);
int vc_cfg_get_file(char *key, char **file);

int vc_cfg_istype(char *key, int type);

#define vc_cfg_isbool(key) vc_cfg_istype(key, VC_CFG_BOOL_T)
#define vc_cfg_isint(key)  vc_cfg_istype(key, VC_CFG_INT_T)
#define vc_cfg_isstr(key)  vc_cfg_istype(key, VC_CFG_STR_T)
#define vc_cfg_islist(key) vc_cfg_istype(key, VC_CFG_LIST_T)

int vc_cfg_get_xid (char *name, xid_t *xid);
int vc_cfg_get_name(xid_t xid, char **name);
int vc_cfg_get_bool(char *name, char *key, int *value);
int vc_cfg_set_bool(char *name, char *key, int value);
int vc_cfg_get_int (char *name, char *key, int *value);
int vc_cfg_set_int (char *name, char *key, int value);
int vc_cfg_get_str (char *name, char *key, char **value);
int vc_cfg_set_str (char *name, char *key, char *value);
int vc_cfg_get_list(char *name, char *key, char **value);
int vc_cfg_set_list(char *name, char *key, char *value);

/* msg.c */
#define vc_vsnprintf _lucid_vsnprintf
#define vc_snprintf  _lucid_snprintf
#define vc_vasprintf _lucid_vasprintf
#define vc_asprintf  _lucid_asprintf
#define vc_vdprintf  _lucid_vdprintf
#define vc_dprintf   _lucid_dprintf
#define vc_vprintf   _lucid_vprintf
#define vc_printf    _lucid_printf

extern const char *vc_argv0;

#define VC_INIT_ARGV0 vc_argv0 = argv[0];

void vc_warn  (const char *fmt, /*args*/ ...);
void vc_warnp (const char *fmt, /*args*/ ...);
void vc_err   (const char *fmt, /*args*/ ...);
void vc_errp  (const char *fmt, /*args*/ ...);
void vc_abort (const char *fmt, /*args*/ ...);
void vc_abortp(const char *fmt, /*args*/ ...);

#define vc_warnf(fmt, ...)   vc_warn  ("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define vc_warnfp(fmt, ...)  vc_warnp ("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define vc_errf(fmt, ...)    vc_err   ("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define vc_errfp(fmt, ...)   vc_errp  ("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define vc_abortf(fmt, ...)  vc_abort ("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define vc_abortfp(fmt, ...) vc_abortp("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)

/* lists.c */
extern const flist64_t vc_bcaps_list[];
extern const flist64_t vc_ccaps_list[];
extern const flist64_t vc_cflags_list[];
extern const flist32_t vc_vhiname_list[];
extern const flist32_t vc_iattr_list[];
extern const flist32_t vc_rlimit_list[];
extern const flist64_t vc_nflags_list[];
extern const flist32_t vc_sched_list[];

/* misc.c */
int vc_secure_chdir(int rootfd, int cwdfd, char *dir);
pid_t vc_waitpid(pid_t pid);

/* ns.c */
int vc_ns_new(xid_t xid);
int vc_str_to_fstab(char *str, char **src, char **dst, char **type, char **data);

/* nx.c */
int vc_nx_exists(nid_t nid);
int vc_nx_new(nid_t nid);
int vc_nx_release(nid_t nid);
int vc_nx_add_addr(nid_t nid, char *cidr);
int vc_nx_rem_addr(nid_t nid, char *cidr);

#define VC_SECURE_NFLAGS 0

/* vx.c */
int vc_vx_exists(xid_t xid);
int vc_vx_new(xid_t xid);
int vc_vx_release(xid_t xid);
uint64_t vc_str_to_rlim(char *str);
char *vc_rlim_to_str(uint64_t lim);

#define VC_INSECURE_BCAPS ( \
	(1<<CAP_LINUX_IMMUTABLE) | \
	(1<<CAP_NET_BROADCAST) | \
	(1<<CAP_NET_ADMIN) | \
	(1<<CAP_NET_RAW) | \
	(1<<CAP_IPC_LOCK) | \
	(1<<CAP_IPC_OWNER) | \
	(1<<CAP_SYS_MODULE) | \
	(1<<CAP_SYS_RAWIO) | \
	(1<<CAP_SYS_PACCT) | \
	(1<<CAP_SYS_ADMIN) | \
	(1<<CAP_SYS_NICE) | \
	(1<<CAP_SYS_RESOURCE) | \
	(1<<CAP_SYS_TIME) | \
	(1<<CAP_MKNOD) | \
	(1<<CAP_AUDIT_CONTROL) | \
	(1<<CAP_CONTEXT) )

#define VC_SECURE_CCAPS ( \
	VXC_SET_UTSNAME | \
	VXC_RAW_ICMP )

#define VC_SECURE_CFLAGS ( \
	VXF_HIDE_NETIF )

#endif
