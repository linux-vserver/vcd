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
#include <inttypes.h>

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
vxdb_result *dbr = NULL;

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

	int i, rc;

	for (i = 0; table->columns[i]; i++)
		stralloc_catf(sa, "%s%s", table->columns[i],
				table->columns[i+1] ? ", " : ");");

	for (i = 0; table->unique[i]; i++)
		stralloc_catf(sa, "CREATE UNIQUE INDEX 'u_%s_%s' ON '%s' (%s);",
				table->name,
				table->unique[i],
				table->name,
				table->unique[i]);

	stralloc_cats(sa, "COMMIT TRANSACTION;");

	char *sql = stralloc_finalize(sa);
	stralloc_free(sa);

	rc = vxdb_exec(sql);

	if (rc != VXDB_OK)
		log_error_and_die("could not create table '%s': %s",
				table->name, vxdb_errmsg(vxdb));

	mem_free(sql);

	/* add onboot group on groups table creation only */
	if (str_equal(table->name, "groups")) {
		rc = vxdb_exec(
				"BEGIN EXCLUSIVE TRANSACTION;"
				"INSERT INTO '%s' (gid, name) VALUES (1, 'onboot');"
				"COMMIT TRANSACTION;",
				table->name);

		if (rc != VXDB_OK)
			log_error_and_die("could not insert data on creation of table '%s': %s",
					table->name, vxdb_errmsg(vxdb));
	}
}

static
void vxdb_create_index(const char *table, const char *columns)
{
	log_info("creating unique index for (%s) in table '%s'",
			columns, table);

	int rc = vxdb_exec("CREATE UNIQUE INDEX 'u_%s_%s' ON '%s' (%s)",
				table, columns, table, columns);

	if (rc != VXDB_OK)
		log_error_and_die("could not create unique index "
				"for (%s) in table '%s': %s",
				columns, table, vxdb_errmsg(vxdb));
}

static
void vxdb_sanity_check_unique(struct _vxdb_table *table)
{
	vxdb_result *dbr;

	int i, rc;

	for (i = 0; table->unique[i]; i++) {
		rc = vxdb_prepare(&dbr,
				"SELECT name FROM sqlite_master "
				"WHERE name = 'u_%s_%s' AND "
				"tbl_name = '%s' AND type = 'index'",
				table->name, table->unique[i], table->name);

		if (rc != VXDB_OK || vxdb_step(dbr) != VXDB_ROW) {
			vxdb_finalize(dbr);

			log_warn("unique index for (%s) missing in table '%s'",
					table->unique[i], table->name);

			vxdb_create_index(table->name, table->unique[i]);
		}

		else
			vxdb_finalize(dbr);
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

	if (rc != VXDB_OK || vxdb_step(dbr) != VXDB_ROW) {
		vxdb_finalize(dbr);
		vxdb_create_table(table);
	}

	else {
		vxdb_finalize(dbr);
		vxdb_sanity_check_unique(table);
	}
}

static
void vxdb_create_user(const char *name)
{
	log_info("creating user '%s' with empty password", name);

	int rc = VXDB_OK, uid = auth_getnextuid();

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

	if (rc != VXDB_OK)
		log_error_and_die("could not create user '%s': %s",
				name, vxdb_errmsg(vxdb));

	log_info("remember to set a password for user '%s'", name);
}

static
void vxdb_sanity_check(void)
{
	LOG_TRACEME

	int rc, i;
	vxdb_result *dbr;

	/* check database version */
	rc = vxdb_prepare(&dbr, "PRAGMA user_version");

	if (rc != VXDB_OK || vxdb_step(dbr) != VXDB_ROW)
		log_error_and_die("could not get database version: %s",
				vxdb_errmsg(vxdb));

	int vxdb_version = vxdb_column_int(dbr, 0);
	int vxdb_vmajor  = vxdb_version >> 8;
	int vxdb_vminor  = vxdb_version & 0xff;

	vxdb_finalize(dbr);

	/* check foreign database */
	if (vxdb_version == 0) {
		log_warn("no database version found");

		rc = vxdb_prepare(&dbr,
				"SELECT name FROM sqlite_master LIMIT 1");

		if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
			log_error_and_die("cannot create initial schema: "
					"database is not empty, but has no version either");

		else
			log_info("database is empty, creating new one from scratch");

		vxdb_finalize(dbr);
	}

	/* check if manual upgrade is required */
	else if (vxdb_vmajor < VXDB_VERSION_MAJOR)
		log_error_and_die("your database schema needs to be updated manually. "
				"please run 'vxdbupdate'");

	/* check if the user has downgraded */
	else if (vxdb_vmajor > VXDB_VERSION_MAJOR ||
			vxdb_vminor > VXDB_VERSION_MINOR)
		log_error_and_die("found database version %#.4x, "
				"but current is %#.4x. downgrade will not work!",
				vxdb_version, VXDB_VERSION);

	/* notify if minor changes will be made */
	else if (vxdb_vminor < VXDB_VERSION_MINOR)
		log_info("updating minor changes to the database automatically");

	/* check database schema */
	for (i = 0; _vxdb_tables[i].name; i++)
		vxdb_sanity_check_table(&(_vxdb_tables[i]));

	/* set current version after schema check has passed */
	rc = vxdb_exec("PRAGMA user_version = %d", VXDB_VERSION);

	if (rc != VXDB_OK)
		log_error_and_die("could not set database version: %s",
				vxdb_errmsg(vxdb));

	/* check if there are users */
	rc = vxdb_prepare(&dbr, "SELECT uid FROM user LIMIT 1");

	if (rc != VXDB_OK || vxdb_step(dbr) != VXDB_ROW) {
		vxdb_finalize(dbr);
		log_warn("no user exists in the database");
		vxdb_create_user("admin");
	}

	else
		vxdb_finalize(dbr);

	/* check if there is an admin user */
	rc = vxdb_prepare(&dbr, "SELECT uid FROM user WHERE admin = 1 LIMIT 1");

	if (rc != VXDB_OK || vxdb_step(dbr) != VXDB_ROW) {
		vxdb_finalize(dbr);
		log_warn("no admin user exists in the database");
		vxdb_create_user("admin");
	}

	else
		vxdb_finalize(dbr);

	/* check if there is a vshelper user */
	rc = vxdb_prepare(&dbr, "SELECT uid FROM user WHERE name = 'vshelper'");

	if (rc != VXDB_OK || vxdb_step(dbr) != VXDB_ROW) {
		vxdb_finalize(dbr);
		log_warn("no vshelper user exists in the database");
		vxdb_create_user("vshelper");
	}

	else
		vxdb_finalize(dbr);
}

static
void vxdb_trace(void *data, const char *sql)
{
	log_debug("[vxdb] %s", sql);
}

void vxdb_init(void)
{
	LOG_TRACEME

	if (vxdb)
		vxdb_atexit();

	char *datadir  = cfg_getstr(cfg, "datadir");
	char *vxdbfile = str_path_concat(datadir, "vxdb");

	int rc = sqlite3_open(vxdbfile, &vxdb);

	mem_free(vxdbfile);

	if (rc != VXDB_OK) {
		log_error("vxdb_init(%s): %s", vxdbfile, vxdb_errmsg(vxdb));
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

	if (!vxdb)
		return;

	if (sqlite3_close(vxdb) == VXDB_BUSY) {
		/* try to finalize last known statement on exit during vxdb query */
		log_warn("unfinished statements on vxdb shutdown - "
				"trying to finalize");

		vxdb_finalize(dbr);

		if (sqlite3_close(vxdb) == VXDB_BUSY) {
			log_alert("unable to finalize unfinished statements");
			log_alert("this is a bug - your database will be corrupted");
			log_alert("please report to %s", PACKAGE_BUGREPORT);
		}
	}

	vxdb = NULL;
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

	if (rc != VXDB_OK)
		log_warn("vxdb_prepare(%s): %s", sql, vxdb_errmsg(vxdb));

	mem_free(sql);

	return rc;
}

int vxdb_step(vxdb_result *dbr)
{
	LOG_TRACEME

	int rc = sqlite3_step(dbr);

	switch (rc) {
		case VXDB_DONE:
		case VXDB_ROW:
		case VXDB_OK:
			break;

		case VXDB_BUSY:
			/* the timeout handler will sleep */
			return vxdb_step(dbr);

		default:
			log_warn("vxdb_step: %s", vxdb_errmsg(vxdb));
			break;
	}

	return rc;
}

int vxdb_finalize(vxdb_result *dbr)
{
	LOG_TRACEME
	return sqlite3_finalize(dbr);
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

	if (rc != VXDB_OK)
		log_warn("vxdb_exec(%s): %s", sql, vxdb_errmsg(vxdb));

	mem_free(sql);

	return rc;
}

const char *vxdb_column_text(vxdb_result *dbr, int col)
{
	const unsigned char *text = sqlite3_column_text(dbr, col);
	return (const char *) text;
}

int vxdb_column_int(vxdb_result *dbr, int col)
{
	const char *text = vxdb_column_text(dbr, col);
	int result = 0;

	sscanf(text, "%d", &result);

	return result;
}

int32_t vxdb_column_int32(vxdb_result *dbr, int col)
{
	const char *text = vxdb_column_text(dbr, col);
	int32_t result = 0;

	sscanf(text, "%" SCNi32, &result);

	return result;
}

uint32_t vxdb_column_uint32(vxdb_result *dbr, int col)
{
	const char *text = vxdb_column_text(dbr, col);
	uint32_t result = 0;

	sscanf(text, "%" SCNu32, &result);

	return result;
}

int64_t vxdb_column_int64(vxdb_result *dbr, int col)
{
	const char *text = vxdb_column_text(dbr, col);
	int64_t result = 0;

	sscanf(text, "%" SCNi64, &result);

	return result;
}

uint64_t vxdb_column_uint64(vxdb_result *dbr, int col)
{
	const char *text = vxdb_column_text(dbr, col);
	uint64_t result = 0;

	sscanf(text, "%" SCNu64, &result);

	return result;
}

xid_t vxdb_getxid(const char *name)
{
	LOG_TRACEME

	int rc;
	xid_t xid;

	if (!validate_name(name))
		return 0;

	rc = vxdb_prepare(&dbr,
			"SELECT xid FROM xid_name_map WHERE name = '%s'", name);

	if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
		xid = vxdb_column_uint32(dbr, 0);
	else
		xid = 0;

	vxdb_finalize(dbr);
	return xid;
}

char *vxdb_getname(xid_t xid)
{
	LOG_TRACEME

	int rc;
	char *name;

	if (!validate_xid(xid))
		return NULL;

	rc = vxdb_prepare(&dbr,
			"SELECT name FROM xid_name_map WHERE xid = %d", xid);

	if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
		name = str_dup(vxdb_column_text(dbr, 0));
	else
		name = NULL;

	vxdb_finalize(dbr);
	return name;
}

char *vxdb_getvdir(const char *name)
{
	LOG_TRACEME

	int rc;
	char *vbasedir, *vdir = NULL;
	xid_t xid;

	if ((xid = vxdb_getxid(name))) {
		rc = vxdb_prepare(&dbr, "SELECT vdir FROM vdir WHERE xid = %d", xid);

		if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
			vdir = str_dup(vxdb_column_text(dbr, 0));
		else
			vdir = NULL;

		vxdb_finalize(dbr);
	}

	if (str_isempty(vdir)) {
		vbasedir = cfg_getstr(cfg, "vbasedir");
		vdir = str_path_concat(vbasedir, name);
	}

	return vdir;
}
