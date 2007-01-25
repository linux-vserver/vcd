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
#include <stdlib.h>
#include <vserver.h>
#include <lucid/chroot.h>
#include <lucid/log.h>

#include "cmd.h"

void cmd_login(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *result;
	xid_t xid;
	char *vdir;
	char *av[] = { "/bin/sh", NULL };

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

	if (ns_enter(xid, 0) == -1)
		log_perror_and_die("vx_enter_namespace");

	if (chroot_secure_chdir(vdir, "/") == -1)
		log_perror_and_die("chroot_secure_chdir");

	if (chroot(".") == -1)
		log_perror_and_die("chroot");

	if (nx_migrate(xid) == -1)
		log_perror_and_die("nx_migrate");

	if (vx_migrate(xid, NULL) == -1)
		log_perror_and_die("vx_migrate");

	do_vlogin(2, av);
}
