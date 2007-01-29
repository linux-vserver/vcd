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

#include <stdlib.h>

#include "cfg.h"
#include "validate.h"
#include "vxdb.h"

#define _LUCID_PRINTF_MACROS
#include <lucid/mem.h>
#include <lucid/log.h>
#include <lucid/printf.h>
#include <lucid/str.h>


sqlite3 *vxdb = NULL;

static
void vxdb_sanity_check(void)
{
	LOG_TRACEME

	int rc;
	vxdb_result *dbr;

	rc = vxdb_prepare(&dbr, "SELECT uid FROM user WHERE admin = 1");

	if (rc != SQLITE_OK || vxdb_step(dbr) != SQLITE_ROW)
		log_warn("No admin user found");

	sqlite3_finalize(dbr);

	rc = vxdb_prepare(&dbr, "SELECT uid FROM user WHERE name = 'vshelper'");

	if (rc != SQLITE_OK || vxdb_step(dbr) != SQLITE_ROW)
		log_warn("No vshelper user found");

	sqlite3_finalize(dbr);
}

static
void vxdb_trace(void *data, const char *sql)
{
	log_debug("[vxdb] %s", sql);
}

void vxdb_init(void)
{
	LOG_TRACEME

	int rc;

	if (vxdb)
		return;

	char *datadir  = cfg_getstr(cfg, "datadir");
	char *vxdbfile = str_path_concat(datadir, "vxdb");

	rc = sqlite3_open(vxdbfile, &vxdb);

	mem_free(vxdbfile);

	if (rc != SQLITE_OK) {
		log_error("sqlite3_open: %s", sqlite3_errmsg(vxdb));
		sqlite3_close(vxdb);
		exit(EXIT_FAILURE);
	}

	sqlite3_busy_timeout(vxdb, 500);
	sqlite3_trace(vxdb, vxdb_trace, NULL);

	vxdb_sanity_check();
}

void vxdb_atexit(void)
{
	LOG_TRACEME
	sqlite3_close(vxdb);
}

int vxdb_prepare(vxdb_result **dbr, const char *fmt, ...)
{
	LOG_TRACEME

	va_list ap;
	char *sql;
	int rc;

	va_start(ap, fmt);
	vasprintf(&sql, fmt, ap);
	va_end(ap);

	rc = sqlite3_prepare(vxdb, sql, -1, dbr, NULL);

	if (rc != SQLITE_OK)
		log_warn("vxdb_prepare(%s): %s", sql, sqlite3_errmsg(vxdb));

	mem_free(sql);

	return rc;
}

int vxdb_step(vxdb_result *dbr)
{
	LOG_TRACEME

	int rc = sqlite3_step(dbr);

	switch (rc) {
		case SQLITE_DONE:
		case SQLITE_ROW:
		case SQLITE_OK:
			break;

		case SQLITE_BUSY:
			/* the timeout handler will sleep */
			return vxdb_step(dbr);

		default:
			log_warn("vxdb_step: %s", sqlite3_errmsg(vxdb));
			break;
	}

	return rc;
}

int vxdb_exec(const char *fmt, ...)
{
	LOG_TRACEME

	va_list ap;
	char *sql;
	int rc;

	va_start(ap, fmt);
	vasprintf(&sql, fmt, ap);
	va_end(ap);

	rc = sqlite3_exec(vxdb, sql, NULL, NULL, NULL);

	mem_free(sql);

	if (rc != SQLITE_OK)
		log_warn("vxdb_exec(%s): %s", sql, sqlite3_errmsg(vxdb));

	return rc;
}

xid_t vxdb_getxid(const char *name)
{
	LOG_TRACEME

	int rc;
	xid_t xid;
	vxdb_result *dbr;

	if (!validate_name(name))
		return 0;

	rc = vxdb_prepare(&dbr,
			"SELECT xid FROM xid_name_map WHERE name = '%s'", name);

	if (rc == SQLITE_OK && vxdb_step(dbr) == SQLITE_ROW)
		xid = sqlite3_column_int(dbr, 0);
	else
		xid = 0;

	sqlite3_finalize(dbr);
	return xid;
}

char *vxdb_getname(xid_t xid)
{
	LOG_TRACEME

	int rc;
	char *name;
	vxdb_result *dbr;

	if (!validate_xid(xid))
		return NULL;

	rc = vxdb_prepare(&dbr,
			"SELECT name FROM xid_name_map WHERE xid = '%d'", xid);

	if (rc == SQLITE_OK && vxdb_step(dbr) == SQLITE_ROW)
		name = str_dup(sqlite3_column_text(dbr, 0));
	else
		name = NULL;

	sqlite3_finalize(dbr);
	return name;
}

char *vxdb_getvdir(const char *name)
{
	LOG_TRACEME

	int rc;
	char *vbasedir, *vdir;
	vxdb_result *dbr;
	xid_t xid;

	if ((xid = vxdb_getxid(name))) {
		rc = vxdb_prepare(&dbr, "SELECT vdir FROM vdir WHERE xid = '%d'", xid);

		if (rc == SQLITE_OK && vxdb_step(dbr) == SQLITE_ROW)
			vdir = str_dup(sqlite3_column_text(dbr, 0));
		else
			vdir = NULL;

		sqlite3_finalize(dbr);
	}

	if (str_isempty(vdir)) {
		vbasedir = cfg_getstr(cfg, "vbasedir");
		vdir = str_path_concat(vbasedir, name);
	}

	return vdir;
}
