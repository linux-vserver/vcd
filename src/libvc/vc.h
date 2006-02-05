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

#ifndef _COMMANDS_COMMNADS_H
#define _COMMANDS_COMMANDS_H

#include <vserver.h>

#include "list.h"

/* lists */
extern const list64_t bcaps_list[];
extern const list64_t ccaps_list[];
extern const list64_t cflags_list[];
extern const list32_t vhiname_list[];
extern const list32_t iattr_list[];
extern const list32_t rlimit_list[];
extern const list64_t nflags_list[];
extern const list32_t sched_list[];

/* dx.c */
int vc_dx_add_path(xid_t xid, char *path);
int vc_dx_rem_path(xid_t xid, char *path);
int vc_dx_get_limit(xid_t xid, char *path, uint32_t *spaceu, uint32_t *spacet,
                 uint32_t *inodesu, uint32_t *inodest, uint32_t *reserved);
int vc_dx_set_limit(xid_t xid, char *path, uint32_t spaceu, uint32_t spacet,
                 uint32_t inodesu, uint32_t inodest, uint32_t reserved);

/* in.c */
int vc_in_get_xid(char *file, xid_t *xid);
int vc_in_get_attr(char *file, char **flagstr, uint32_t *flags);
int vc_in_set_xid(char *file, xid_t xid);
int vc_in_set_attr(char *file, char *flagstr);

/* ns.c */
int vc_ns_new(xid_t xid);
int vc_ns_migrate(xid_t xid);

/* nx.c */
int vc_nx_exists(nid_t nid);
int vc_nx_create(nid_t nid, char *flagstr);
int vc_nx_new(nid_t nid, char *flagstr);
int vc_nx_migrate(nid_t nid);
int vc_nx_get_flags(nid_t nid, char **flagstr, uint64_t *flags);
int vc_nx_set_flags(nid_t nid, char *flagstr);
int vc_nx_add_addr(nid_t nid, char *cidr);
int vc_nx_rem_addr(nid_t nid, char *cidr);

/* task.c */
int vc_task_nid(pid_t pid, xid_t *xid);
int vc_task_xid(pid_t pid, nid_t *nid);

/* vx.c */
int vc_vx_exists(xid_t xid);
int vc_vx_create(xid_t xid, char *flagstr);
int vc_vx_new(xid_t xid, char *flagstr);
int vc_vx_migrate(xid_t xid);
int vc_vx_kill(xid_t xid, pid_t pid, int sig);
int vc_vx_wait(xid_t xid);
int vc_vx_get_bcaps(xid_t xid, char **flagstr, uint64_t *flags);
int vc_vx_get_ccaps(xid_t xid, char **flagstr, uint64_t *flags);
int vc_vx_get_flags(xid_t xid, char **flagstr, uint64_t *flags);
int vc_vx_get_limit(xid_t xid, char *type,
                    uint64_t *min, uint64_t *soft, uint64_t *max);
int vc_vx_get_uname(xid_t xid, char *key, char **value);
int vc_vx_set_bcaps(xid_t xid, char *flagstr);
int vc_vx_set_ccaps(xid_t xid, char *flagstr);
int vc_vx_set_flags(xid_t xid, char *flagstr);
int vc_vx_set_limit(xid_t xid, char *type,
                    uint32_t min, uint32_t soft, uint32_t max);
int vc_vx_set_uname(xid_t xid, char *key, char *value);

#endif
