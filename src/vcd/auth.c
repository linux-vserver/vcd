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

#include "auth.h"
#include "lists.h"
#include "validate.h"
#include "vxdb.h"

#include <lucid/mem.h>
#include <lucid/log.h>
#include <lucid/str.h>
#include <lucid/whirlpool.h>

int auth_isvalid(const char *user, const char *pass)
{
	LOG_TRACEME

	int rc;
	char *whirlpool_pass;

	if (mem_cmp(pass, "WHIRLPOOLENC//", 14) == 0)
		whirlpool_pass = str_dup(pass+14);
	else
		whirlpool_pass = whirlpool_digest(pass);

	rc = vxdb_prepare(&dbr,
			"SELECT uid FROM user WHERE name = '%s' AND password = '%s'",
			user, whirlpool_pass);

	mem_free(whirlpool_pass);

	if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
		rc = 1;
	else
		rc = 0;

	vxdb_finalize(dbr);

	return rc;
}

int auth_isadmin(const char *user)
{
	LOG_TRACEME

	int rc;

	rc = vxdb_prepare(&dbr,
			"SELECT uid FROM user WHERE name = '%s' AND admin = 1",
			user);

	if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
		rc = 1;
	else
		rc = 0;

	vxdb_finalize(dbr);

	return rc;
}

static
int auth_hascapability(const char *user, uint64_t cap)
{
	LOG_TRACEME

	const char *buf;
	int uid, rc;

	if (!(uid = auth_getuid(user)))
		return 0;

	if (!(buf = flist64_getkey(vcd_caps_list, cap)))
		return 0;

	rc = vxdb_prepare(&dbr,
			"SELECT uid FROM user_caps WHERE uid = %d and cap = '%s'",
			uid, buf);

	if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
		rc = 1;
	else
		rc = 0;

	vxdb_finalize(dbr);

	return rc;
}

int auth_capable(const char *user, uint64_t caps)
{
	LOG_TRACEME

	int i;

	for (i = 0; vcd_caps_list[i].key; i++)
		if (caps & vcd_caps_list[i].val)
			if (!auth_hascapability(user, vcd_caps_list[i].val))
				return 0;

	return 1;
}

int auth_isowner(const char *user, const char *name)
{
	LOG_TRACEME

	int uid, rc;
	xid_t xid;

	if ((uid = auth_getuid(user)) == 0)
		return 0;

	if (auth_isadmin(user))
		return 1;

	if ((xid = vxdb_getxid(name)) == 0)
		return 0;

	rc = vxdb_prepare(&dbr,
			"SELECT uid FROM xid_uid_map WHERE uid = %d AND xid = %d",
			uid, xid);

	if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
		rc = 1;
	else
		rc = 0;

	vxdb_finalize(dbr);

	return rc;
}

int auth_getuid(const char *user)
{
	LOG_TRACEME

	int uid, rc;

	if (!validate_username(user))
		return 0;

	rc = vxdb_prepare(&dbr,
			"SELECT uid FROM user WHERE name = '%s'",
			user);

	if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
		uid = vxdb_column_int(dbr, 0);
	else
		uid = 0;

	vxdb_finalize(dbr);

	return uid;
}

int auth_getnextuid(void)
{
	LOG_TRACEME

	int uid, rc;

	rc = vxdb_prepare(&dbr,
			"SELECT uid FROM user ORDER BY uid DESC LIMIT 1");

	if (rc == VXDB_OK && vxdb_step(dbr) == VXDB_ROW)
		uid = vxdb_column_int(dbr, 0);
	else
		uid = 0;

	vxdb_finalize(dbr);

	return uid + 1;
}
