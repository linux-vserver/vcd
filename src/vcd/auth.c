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

#include <string.h>

#include "xmlrpc.h"

#include "auth.h"
#include "vxdb.h"

int auth_isvalid(XMLRPC_REQUEST r)
{
	int rc = 0;
	dbi_result dbr;
	XMLRPC_VALUE request, auth;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	
	char *username = XMLRPC_VectorGetStringWithID(auth, "username");
	char *password = XMLRPC_VectorGetStringWithID(auth, "password");
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT password FROM user WHERE name = '%s'",
		username);
	
	if (!dbr)
		return 0;
	
	if (dbi_result_get_numrows(dbr) != 1)
		goto out;
	
	dbi_result_first_row(dbr);
	
	if (strcmp(dbi_result_get_string(dbr, "password"), password) == 0)
		rc = 1;
	
out:
	dbi_result_free(dbr);
	return rc;
}

int auth_isadmin(XMLRPC_REQUEST r)
{
	int rc = 0;
	dbi_result dbr;
	XMLRPC_VALUE request, auth;
	
	if (!auth_isvalid(r))
		return 0;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	
	char *username = (char *) XMLRPC_VectorGetStringWithID(auth, "username");
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT admin FROM user WHERE name = '%s' AND admin = 1",
		username);
	
	if (!dbr)
		return 0;
	
	if (dbi_result_get_numrows(dbr) > 0)
		rc = 1;
	
	dbi_result_free(dbr);
	return rc;
}

int auth_isowner(XMLRPC_REQUEST r)
{
	int rc = 0;
	xid_t xid;
	dbi_result dbr;
	XMLRPC_VALUE request, auth, params;
	
	if (!auth_isvalid(r))
		return 0;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	params  = XMLRPC_VectorNext(request);
	
	char *username = XMLRPC_VectorGetStringWithID(auth, "username");
	char *name     = XMLRPC_VectorGetStringWithID(params, "name");
	
	if (vxdb_getxid(name, &xid) == -1)
		return 0;
	
	int uid = auth_getuid(r);
	
	if (uid == 0)
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

int auth_getuid(XMLRPC_REQUEST r)
{
	int uid = 0;
	dbi_result dbr;
	XMLRPC_VALUE request, auth;
	
	if (!auth_isvalid(r))
		return 0;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	
	char *username = XMLRPC_VectorGetStringWithID(auth, "username");
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT uid FROM user WHERE name = '%s'",
		username);
	
	if (!dbr)
		return 0;
	
	if (dbi_result_get_numrows(dbr) > 0) {
		dbi_result_first_row(dbr);
		uid = dbi_result_get_int(dbr, "uid");
	}
	
	dbi_result_free(dbr);
	return uid;
}
