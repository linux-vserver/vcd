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

#include "lucid.h"

#include "auth.h"
#include "lists.h"
#include "vxdb.h"

int auth_isvalid(const char *user, const char *pass)
{
	int rc = 0;
	dbi_result dbr;
	char *sha1_pass = sha1_digest(pass);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT uid FROM user WHERE name = '%s' AND password = '%s'",
		user, sha1_pass);
	
	free(sha1_pass);
	
	if (!dbr)
		return 0;
	
	if (dbi_result_get_numrows(dbr) > 0)
		rc = 1;
	
	dbi_result_free(dbr);
	return rc;
}

int auth_isadmin(const char *user)
{
	int rc = 0;
	dbi_result dbr;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT uid FROM user WHERE name = '%s' AND admin = 1",
		user);
	
	if (!dbr)
		return 0;
	
	if (dbi_result_get_numrows(dbr) > 0)
		rc = 1;
	
	dbi_result_free(dbr);
	return rc;
}

static
int auth_hascapability(const char *user, uint64_t cap)
{
	dbi_result dbr;
	const char *buf;
	int uid, rc = 0;
	
	if (!(uid = auth_getuid(user)))
		return 0;
	
	if (!(buf = flist64_getkey(vcd_caps_list, cap)))
		return 0;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT uid FROM user_caps WHERE uid = %d and cap = '%s'",
		uid, buf);
	
	if (!dbr)
		return 0;
	
	if (dbi_result_get_numrows(dbr) > 0)
		rc = 1;
	
	dbi_result_free(dbr);
	return rc;
}

int auth_capable(const char *user, uint64_t caps)
{
	int i;
	
	for (i = 0; vcd_caps_list[i].key; i++)
		if (caps & vcd_caps_list[i].val)
			if (!auth_hascapability(user, vcd_caps_list[i].val))
				return 0;
	
	return 1;
}

int auth_isowner(const char *user, const char *name)
{
	int uid, rc = 0;
	xid_t xid;
	dbi_result dbr;
	
	if ((uid = auth_getuid(user)) == 0)
		return 0;
	
	if (auth_isadmin(user))
		return 1;
	
	if ((xid = vxdb_getxid(name)) == 0)
		return 0;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT uid FROM xid_uid_map WHERE uid = %d AND xid = %d",
		uid, xid);
	
	if (!dbr)
		return 0;
	
	if (dbi_result_get_numrows(dbr) > 0)
		rc = 1;
	
	dbi_result_free(dbr);
	return rc;
}

int auth_getuid(const char *user)
{
	int uid = 0;
	dbi_result dbr;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT uid FROM user WHERE name = '%s'",
		user);
	
	if (!dbr)
		return 0;
	
	if (dbi_result_get_numrows(dbr) > 0) {
		dbi_result_first_row(dbr);
		uid = dbi_result_get_int(dbr, "uid");
	}
	
	dbi_result_free(dbr);
	return uid;
}
