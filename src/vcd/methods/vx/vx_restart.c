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

#include "xmlrpc.h"

#include "methods.h"

/* vx.restart(string name[, int wait]) */
XMLRPC_VALUE m_vx_restart(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE params = method_get_params(r);
	XMLRPC_VALUE response;
	
	XMLRPC_VALUE reboot = XMLRPC_VectorGetValueWithID(params, "reboot");
	
	if (!reboot)
		XMLRPC_AddValueToVector(params, XMLRPC_CreateValueInt("reboot", 1));
	else
		XMLRPC_SetValueInt(reboot, 1);
	
	response = m_vx_stop(s, r, d);
	
	const char *fault_string = XMLRPC_VectorGetStringWithID(response, "faultString");
	int fault_code           = XMLRPC_VectorGetIntWithID(response, "faultCode");
	
	if (fault_string || fault_code != 0)
		return response;
	
	return params;
}
