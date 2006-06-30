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

#include <string.h>

#include "lucid.h"

#include "cfg.h"
#include "log.h"
#include "validate.h"
#include "vxdb.h"

dbi_conn vxdb = NULL;

void vxdb_init(void)
{
	if (vxdb)
		return;
	
	char *datadir = cfg_getstr(cfg, "datadir");
	
	dbi_initialize(NULL);
	vxdb = dbi_conn_new("sqlite3");
	
	if (!vxdb)
		log_error_and_die("Unable to load sqlite3 driver");
	
	dbi_conn_set_option(vxdb, "dbname", "vxdb");
	dbi_conn_set_option(vxdb, "sqlite3_dbdir", datadir);
	
	if (dbi_conn_connect(vxdb) < 0)
		log_error_and_die("Could not open vxdb");
}

void vxdb_close(void)
{
	if (vxdb)
		dbi_conn_close(vxdb);
	
	dbi_shutdown();
}

xid_t vxdb_getxid(const char *name)
{
	xid_t xid;
	dbi_result dbr;
	
	if (!validate_name(name))
		return 0;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT xid FROM xid_name_map WHERE name = '%s'",
		name);
	
	if (!dbr)
		return 0;
	
	if (dbi_result_get_numrows(dbr) < 1)
		xid = 0;
	
	else {
		dbi_result_first_row(dbr);
		xid = dbi_result_get_int(dbr, "xid");
	}
	
	dbi_result_free(dbr);
	return xid;
}

char *vxdb_getname(xid_t xid)
{
	char *name;
	dbi_result dbr;
	
	if (!validate_xid(xid))
		return NULL;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT name FROM xid_name_map WHERE xid = '%d'",
		xid);
	
	if (!dbr)
		return NULL;
	
	if (dbi_result_get_numrows(dbr) < 1)
		name = NULL;
	
	else {
		dbi_result_first_row(dbr);
		name = strdup(dbi_result_get_string(dbr, "name"));
	}
	
	dbi_result_free(dbr);
	return name;
}
