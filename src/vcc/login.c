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

#include <unistd.h>
#include <stdlib.h>
#include <vserver.h>
#include <lucid/argv.h>
#include <lucid/chroot.h>

#include "cmd.h"
#include "msg.h"

void cmd_login(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *result;
	xid_t xid;
	char *vdir;
	
	char cmd[] = "/bin/login"; /* this form prevents storage in ro section */
	char *av[2];
	int ac;
	
	ac = argv_from_str(cmd, av, 2);
	
	result = xmlrpc_client_call(env, uri, "vxdb.xid.get",
		SIGNATURE("{s:s}"),
		"name", name);
	return_if_fault(env);
	
	xmlrpc_decompose_value(env, result, "i", &xid);
	return_if_fault(env);
	
	xmlrpc_DECREF(result);
	
	result = xmlrpc_client_call(env, uri, "vxdb.vdir.get",
		SIGNATURE("{s:s}"),
		"name", name);
	return_if_fault(env);
	
	xmlrpc_decompose_value(env, result, "s", &vdir);
	return_if_fault(env);
	
	xmlrpc_DECREF(result);
	
	if (vx_enter_namespace(xid) == -1)
		perr("vx_enter_namespace");
	
	if (chroot_secure_chdir(vdir, "/") == -1)
		perr("chroot_secure_chdir");
	
	if (chroot(".") == -1)
		perr("chroot");
	
	if (nx_migrate(xid) == -1)
		perr("nx_migrate");
	
	if (vx_migrate(xid, NULL) == -1)
		perr("vx_migrate");
	
	do_vlogin(ac, av);
}
