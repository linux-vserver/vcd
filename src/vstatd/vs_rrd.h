// Copyright 2006 Remo Lemma <coloss7@gmail.com>
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

#ifndef _VSTATD_RRD
#define _VSTATD_RRD

#include <time.h>
#include "vstats.h"

#define UARGC 3

typedef struct {
	char *db;
	char *name;
	int (* func_cr)(xid_t, char *, char *);
	int (* func_up)(xid_t, char *, char *, char *);
} vs_rrd_db;

extern vs_rrd_db RRD_DB[];

time_t vs_rrd_gettime (time_t curtime);
int vs_rrd_check (xid_t xid, char *vname);

int vs_rrd_create_limit (xid_t xid, char *dbname, char *path);
int vs_rrd_create_info (xid_t xid, char *dbname, char *path);
int vs_rrd_create_loadavg (xid_t xid, char *dbname, char *path);
int vs_rrd_create_net (xid_t xid, char *dbname, char *path);

int vs_rrd_update_limit (xid_t xid, char *dbname, char *name, char *path);
int vs_rrd_update_info (xid_t xid, char *dbname, char *name, char *path);
int vs_rrd_update_loadavg (xid_t xid, char *dbname, char *name, char *path);
int vs_rrd_update_net (xid_t xid, char *dbname, char *name, char *path);
#endif
