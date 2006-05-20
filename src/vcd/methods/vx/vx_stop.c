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

#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <vserver.h>
#include <sys/wait.h>

#include "xmlrpc.h"

#include "auth.h"
#include "cfg.h"
#include "log.h"
#include "methods.h"
#include "vxdb.h"

/* stop process:
   1) create new vps killer (background)
   2) run init-style based stop command
   3) wait for vps killer
*/

static xid_t xid = 0;
static char *name = NULL;
static char *errmsg = NULL;

/* vx.stop(string name) */
XMLRPC_VALUE m_vx_stop(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r) && !auth_isowner(r))
		return method_error(MEPERM);
	
	name = XMLRPC_VectorGetStringWithID(params, "name");
	
	if (!name)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	if (vx_get_info(xid, NULL) == -1)
		return method_error(MESTOPPED);
	
	/* stop here */
	
	return params;
}
