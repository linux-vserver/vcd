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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "cfg.h"
#include "log.h"
#include "vxdb.h"

dbi_conn vxdb = NULL;

void vxdb_init(void)
{
	if (vxdb)
		return;
	
	char *dbdir = cfg_getstr(cfg, "vxdb-path");
	
	if (!dbdir)
		log_error_and_die("Unable to load vxdb configuration");
	
	dbi_initialize(NULL);
	vxdb = dbi_conn_new("sqlite3");
	
	if (!vxdb)
		log_error_and_die("Unable to load sqlite3 driver");
	
	dbi_conn_set_option(vxdb, "dbname", "vxdb");
	dbi_conn_set_option(vxdb, "sqlite3_dbdir", dbdir);
	
	if (dbi_conn_connect(vxdb) < 0)
		log_error_and_die("Could not open vxdb");
}

void vxdb_close(void)
{
	if (vxdb)
		dbi_conn_close(vxdb);
	
	dbi_shutdown();
}

int vxdb_getxid(char *name, xid_t *xid)
{
	dbi_result dbr;
	
	if (!name)
		return errno = EINVAL, -1;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT xid FROM xid_name_map WHERE name = '%s'",
		name);
	
	if (dbi_result_get_numrows(dbr) == 0)
		return errno = ENOENT, -1;
	
	dbi_result_first_row(dbr);
	
	if (xid)
		*xid = dbi_result_get_int(dbr, "xid");
	
	return 0;
}

int vxdb_getname(xid_t xid, char **name)
{
	dbi_result dbr;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT name FROM xid_name_map WHERE xid = '%d'",
		xid);
	
	if (dbi_result_get_numrows(dbr) == 0)
		return errno = ENOENT, -1;
	
	dbi_result_first_row(dbr);
	
	if (name)
		*name = (char *) dbi_result_get_string(dbr, "name");
	
	return 0;
}
