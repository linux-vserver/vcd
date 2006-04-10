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

#include <errno.h>
#include <vserver.h>

#include "xmlrpc.h"

#include "auth.h"
#include "vxdb.h"
#include "xid.h"

XMLRPC_VALUE m_vx_status(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE request, auth, params;
	XMLRPC_VALUE response;
	char *name;
	
	xid_t xid;
	struct vx_info vxi = { 0, 0 };
	
	int run = 1;
	
	request = XMLRPC_RequestGetData(r);
	auth    = XMLRPC_VectorRewind(request);
	params  = XMLRPC_VectorNext(request);
	
	if (!auth_capable(auth, "vx.status"))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	name = (char *) XMLRPC_VectorGetStringWithID(params, "name");
	
	if (!auth_vxowner(auth, name))
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	if (xid_byname(name, &xid) == -1)
		return XMLRPC_UtilityCreateFault(404, "Not Found");
	
	if (vx_get_info(xid, &vxi) == -1) {
		if (errno == ESRCH)
			run = 0;
		else
			return XMLRPC_UtilityCreateFault(500, "Internal Server Error");
	}
	
	response = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
	
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueString("name", name, 0));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueInt("xid", xid));
	XMLRPC_AddValueToVector(response, XMLRPC_CreateValueBoolean("running", run));
	
	return response;
}
