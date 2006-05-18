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
	
	dbi_initialize(NULL);
	vxdb = dbi_conn_new("sqlite3");
	
	if (!vxdb)
		log_error_and_die("Unable to load sqlite3 driver");
	
	dbi_conn_set_option(vxdb, "dbname", "vxdb");
	dbi_conn_set_option(vxdb, "sqlite3_dbdir", dbdir);
	
	if (dbi_conn_connect(vxdb) < 0)
		log_error_and_die("Could not open vxdb");
	
	log_info("Successfully loaded vxdb");
}

void vxdb_close(void)
{
	if (vxdb)
		dbi_conn_close(vxdb);
	
	dbi_shutdown();
}
