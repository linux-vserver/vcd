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

#include "auth.h"
#include "cfg.h"
#include "validate.h"
#include "vxdb.h"
#include "vxdb-tables.h"

#include <lucid/mem.h>
#include <lucid/log.h>
#include <lucid/printf.h>
#include <lucid/str.h>
#include <lucid/stralloc.h>


sqlite3 *vxdb = NULL;

static
void vxdb_create_table(struct _vxdb_table *table)
{
	log_info("creating table '%s'", table->name);

	stralloc_t _sa, *sa = &_sa;
	stralloc_init(sa);

	stralloc_catf(sa,
			"BEGIN EXCLUSIVE TRANSACTION;"
			"CREATE TABLE '%s' (",
			table->name);

	int i;

	for (i = 0; table->columns[i]; i++)
		stralloc_catf(sa, "%s%s", table->columns[i],
				table->columns[i+1] ? ", " : ");");

	for (i = 0; table->unique[i]; i++)
		stralloc_catf(sa, "CREATE UNIQUE INDEX 'u_%s' ON '%s' (%s);",
				table->unique[i],
				table->name,
				table->unique[i]);

	stralloc_cats(sa, "COMMIT TRANSACTION;");

	char *sql = stralloc_finalize(sa);
	stralloc_free(sa);

	int rc = vxdb_exec(sql);

	if (rc != SQLITE_OK)
		log_error_and_die("could not create table '%s': %s",
				table->name, sqlite3_errmsg(vxdb));

	mem_free(sql);
}

static
void vxdb_create_index(const char *table, const char *columns)
{
	log_info("creating unique index for (%s) in table '%s'",
			columns, table);

	int rc = vxdb_exec("CREATE UNIQUE INDEX 'u_%s' ON '%s' (%s)",
				columns, table, columns);

	if (rc != SQLITE_OK)
		log_error_and_die("could not create unique index "
				"for (%s) in table '%s': %s",
				columns, table, sqlite3_errmsg(vxdb));
}

static
void vxdb_sanity_check_unique(struct _vxdb_table *table)
{
	vxdb_result *dbr;

	int i, rc;

	for (i = 0; table->unique[i]; i++) {
		rc = vxdb_prepare(&dbr,
				"SELECT name FROM sqlite_master "
				"WHERE name = '%s' AND tbl_name '%s' AND type = 'index'",
				table->unique[i], table->name);

		if (rc != SQLITE_OK || vxdb_step(dbr) != SQLITE_ROW) {
			log_warn("unique index for (%s) missing in table '%s'",
					table->unique[i], table->name);
			vxdb_create_index(table->name, table->unique[i]);
		}
	}
}

static
void vxdb_sanity_check_table(struct _vxdb_table *table)
{
	vxdb_result *dbr;

	int rc = vxdb_prepare(&dbr,
			"SELECT name FROM sqlite_master "
			"WHERE name = '%s' AND type = 'table'",
			table->name);

	if (rc != SQLITE_OK || vxdb_step(dbr) != SQLITE_ROW) {
		log_warn("table '%s' does not exist", table->name);
		vxdb_create_table(table);
	}

	else
		vxdb_sanity_check_unique(table);
}

static
void vxdb_create_user(const char *name)
{
	log_info("creating user '%s' with empty password", name);

	int rc = SQLITE_OK, uid = auth_getnextuid();

	if (str_equal(name, "admin")) {
		rc = vxdb_exec(
				"BEGIN EXCLUSIVE TRANSACTION;"
				"INSERT INTO user (uid, name, password, admin) "
				"VALUES (%d, 'admin', '*', 1);"
				"INSERT INTO user_caps VALUES(%d, 'AUTH');"
				"INSERT INTO user_caps VALUES(%d, 'DLIM');"
				"INSERT INTO user_caps VALUES(%d, 'INIT');"
				"INSERT INTO user_caps VALUES(%d, 'MOUNT');"
				"INSERT INTO user_caps VALUES(%d, 'NET');"
				"INSERT INTO user_caps VALUES(%d, 'BCAP');"
				"INSERT INTO user_caps VALUES(%d, 'CCAP');"
				"INSERT INTO user_caps VALUES(%d, 'CFLAG');"
				"INSERT INTO user_caps VALUES(%d, 'RLIM');"
				"INSERT INTO user_caps VALUES(%d, 'SCHED');"
				"INSERT INTO user_caps VALUES(%d, 'UNAME');"
				"INSERT INTO user_caps VALUES(%d, 'CREATE');"
				"INSERT INTO user_caps VALUES(%d, 'EXEC');"
				"INSERT INTO user_caps VALUES(%d, 'INFO');" /* 15 */
				"COMMIT TRANSACTION;",
				uid, uid, uid, uid, uid, uid, uid,
				uid, uid, uid, uid, uid, uid, uid, uid);
	}

	else if (str_equal(name, "vshelper")) {
		rc = vxdb_exec(
				"BEGIN EXCLUSIVE TRANSACTION;"
				"INSERT INTO user (uid, name, password, admin) "
				"VALUES (%d, 'vshelper', '*', 0);"
				"INSERT INTO user_caps VALUES(%d, 'HELPER');"
				"COMMIT TRANSACTION",
				uid, uid);
	}

	if (rc != SQLITE_OK)
		log_error_and_die("could not create user '%s': %s",
				name, sqlite3_errmsg(vxdb));

	log_info("remember to set a password for user '%s'", name);
}

static
void vxdb_sanity_check(void)
{
	LOG_TRACEME

	int rc, i;
	vxdb_result *dbr;

	/* check database schema */
	for (i = 0; _vxdb_tables[i].name; i++)
		vxdb_sanity_check_table(&(_vxdb_tables[i]));

	/* check if there are users */
	rc = vxdb_prepare(&dbr, "SELECT uid FROM user LIMIT 1");

	if (rc != SQLITE_OK || vxdb_step(dbr) != SQLITE_ROW) {
		log_warn("no user exists in the database");
		vxdb_create_user("admin");
	}

	sqlite3_finalize(dbr);

	/* check if there is an admin user */
	rc = vxdb_prepare(&dbr, "SELECT uid FROM user WHERE admin = 1 LIMIT 1");

	if (rc != SQLITE_OK || vxdb_step(dbr) != SQLITE_ROW) {
		log_warn("no admin user exists in the database");
		vxdb_create_user("admin");
	}

	sqlite3_finalize(dbr);

	/* check if there is a vshelper user */
	rc = vxdb_prepare(&dbr, "SELECT uid FROM user WHERE name = 'vshelper'");

	if (rc != SQLITE_OK || vxdb_step(dbr) != SQLITE_ROW) {
		log_warn("no vshelper user exists in the database");
		vxdb_create_user("vshelper");
	}

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
	char *vbasedir, *vdir = NULL;
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
