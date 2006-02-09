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

/* argv.c */
char **vc_str_to_argv(char *str, int *argc);
void vc_argv_free(int argc, char **argv);

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

int vc_cfg_get_bool(char *name, char *key, int *value);
int vc_cfg_set_bool(char *name, char *key, int value);
int vc_cfg_get_int(char *name, char *key, int *value);
int vc_cfg_set_int(char *name, char *key, int value);
int vc_cfg_get_str(char *name, char *key, char **value);
int vc_cfg_set_str(char *name, char *key, char *value);
int vc_cfg_get_list(char *name, char *key, char **value);
int vc_cfg_set_list(char *name, char *key, char *value);

int vc_cfg_get_xid(char *name, xid_t *xid);
int vc_cfg_get_name(xid_t xid, char **name);

/* dx.c */
int vc_dx_add_path(char *name, char *path);
int vc_dx_rem_path(char *name, char *path);
int vc_dx_get_limit(char *name, char *path,
                    uint32_t *spaceu, uint32_t *spacet,
                    uint32_t *inodesu, uint32_t *inodest,
                    uint32_t *reserved);
int vc_dx_set_limit(char *name, char *path,
                    uint32_t spaceu, uint32_t spacet,
                    uint32_t inodesu, uint32_t inodest,
                    uint32_t reserved);

/* in.c */
int vc_ix_get_name(char *file, char **name);
int vc_ix_get_xid(char *file, xid_t *xid);
int vc_ix_get_attr(char *file, char **flagstr, uint32_t *flags);
int vc_ix_set_name(char *file, char *name);
int vc_ix_set_xid(char *file, xid_t xid);
int vc_ix_set_attr(char *file, char *flagstr);

/* io.c */
int vc_vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int vc_vasprintf(char **ptr, const char *fmt, va_list ap);
int vc_snprintf(char *str, size_t size, const char *fmt, /*args*/ ...);
int vc_asprintf(char **ptr, const char *fmt, /*args*/ ...);
int vc_vdprintf(int fd, const char *fmt, va_list ap);
int vc_dprintf(int fd, const char *fmt, /*args*/ ...);
int vc_vprintf(const char *fmt, va_list ap);
int vc_printf(const char *fmt, /*args*/ ...);

extern const char *vc_argv0;

#define VC_INIT_ARGV0 vc_argv0 = argv[0];

void vc_warn(const char *fmt, /*args*/ ...);
void vc_warnp(const char *fmt, /*args*/ ...);
void vc_err(const char *fmt, /*args*/ ...);
void vc_errp(const char *fmt, /*args*/ ...);

#define vc_warnf(fmt, ...)  vc_warn("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define vc_warnfp(fmt, ...) vc_warnp("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define vc_errf(fmt, ...)   vc_err("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define vc_errfp(fmt, ...)  vc_errp("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)

int vc_readline(int fd, char **line);

/* list.c */
typedef struct {
	char *key;
	uint32_t val;
} vc_list32_t;

typedef struct {
	char *key;
	uint64_t val;
} vc_list64_t;

#define VC_LIST32_START(LIST) const vc_list32_t LIST[] = {
#define VC_LIST32_NODE(PREFIX, NAME) { #NAME, PREFIX ## _ ## NAME },
#define VC_LIST32_END { NULL, 0 } };

#define VC_LIST64_START(LIST) const vc_list64_t LIST[] = {
#define VC_LIST64_NODE(PREFIX, NAME) { #NAME, PREFIX ## _ ## NAME },
#define VC_LIST64_END { NULL, 0 } };

int vc_list32_getval(const vc_list32_t list[], char *key, uint32_t *val);
int vc_list64_getval(const vc_list64_t list[], char *key, uint64_t *val);
int vc_list32_getkey(const vc_list32_t list[], uint32_t val, char **key);
int vc_list64_getkey(const vc_list64_t list[], uint64_t val, char **key);
int vc_list32_parse(char *str, const vc_list32_t list[],
                    uint32_t *flags, uint32_t *mask,
                    char clmod, char delim);
int vc_list64_parse(char *str, const vc_list64_t list[],
                    uint64_t *flags, uint64_t *mask,
                    char clmod, char delim);
int vc_list32_tostr(const vc_list32_t list[], uint32_t val, char **str, char delim);
int vc_list64_tostr(const vc_list64_t list[], uint64_t val, char **str, char delim);

/* lists.c */
extern const vc_list64_t vc_bcaps_list[];
extern const vc_list64_t vc_ccaps_list[];
extern const vc_list64_t vc_cflags_list[];
extern const vc_list32_t vc_vhiname_list[];
extern const vc_list32_t vc_iattr_list[];
extern const vc_list32_t vc_rlimit_list[];
extern const vc_list64_t vc_nflags_list[];
extern const vc_list32_t vc_sched_list[];

/* misc.c */
int vc_secure_chdir(int rootfd, int cwdfd, char *dir);

/* ns.c */
int vc_ns_new(char *name);
int vc_ns_migrate(char *name);

/* nx.c */
int vc_nx_exists(char *name);
int vc_nx_create(char *name, char *flagstr);
int vc_nx_new(char *name, char *flagstr);
int vc_nx_migrate(char *name);
int vc_nx_get_flags(char *name, char **flagstr, uint64_t *flags);
int vc_nx_set_flags(char *name, char *flagstr);
int vc_nx_add_addr(char *name, char *cidr);
int vc_nx_rem_addr(char *name, char *cidr);

/* task.c */
int vc_task_name(pid_t pid, char **name);
int vc_task_nid(pid_t pid, xid_t *xid);
int vc_task_xid(pid_t pid, nid_t *nid);

/* vx.c */
int vc_vx_exists(char *name);
int vc_vx_create(char *name, char *flagstr);
int vc_vx_new(char *name, char *flagstr);
int vc_vx_migrate(char *name);
int vc_vx_kill(char *name, pid_t pid, int sig);
int vc_vx_wait(char *name);
int vc_vx_get_bcaps(char *name, char **flagstr, uint64_t *flags);
int vc_vx_get_ccaps(char *name, char **flagstr, uint64_t *flags);
int vc_vx_get_flags(char *name, char **flagstr, uint64_t *flags);
int vc_vx_get_limit(char *name, char *type,
                    uint64_t *min, uint64_t *soft, uint64_t *max);
int vc_vx_get_uname(char *name, char *key, char **value);
int vc_vx_set_bcaps(char *name, char *flagstr);
int vc_vx_set_ccaps(char *name, char *flagstr);
int vc_vx_set_flags(char *name, char *flagstr);
int vc_vx_set_limit(char *name, char *type,
                    uint32_t min, uint32_t soft, uint32_t max);
int vc_vx_set_uname(char *name, char *key, char *value);

#endif
