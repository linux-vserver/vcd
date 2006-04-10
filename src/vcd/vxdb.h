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

#ifndef _METHODS_VXDB_H
#define _METHODS_VXDB_H

#include "confuse.h"
#include "xmlrpc.h"

extern cfg_opt_t dlimit_OPTS[];
extern cfg_opt_t init_OPTS[];
extern cfg_opt_t nx_addr_OPTS[];
extern cfg_opt_t nx_OPTS[];
extern cfg_opt_t rlimit_OPTS[];
extern cfg_opt_t sched_OPTS[];
extern cfg_opt_t uts_OPTS[];
extern cfg_opt_t vx_OPTS[];
extern cfg_opt_t OPTS[];

cfg_t     *vxdb_open(char *name);
void       vxdb_close(cfg_t *cfg);
int        vxdb_closewrite(cfg_t *cfg);
cfg_opt_t *vxdb_lookup(cfg_t *cfg, char *key, char *title);
int        vxdb_addsec(cfg_t *cfg, char *key, char *title);
int        vxdb_capable(XMLRPC_VALUE auth, char *name, char *key, int write);

#define vxdb_capable_read(auth, name, key)   vxdb_capable(auth, name, key, 0)
#define vxdb_capable_write(auth, name, key)  vxdb_capable(auth, name, key, 1)

#endif
