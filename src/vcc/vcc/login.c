// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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

#include <lucid/chroot.h>
#include <lucid/log.h>
#include <lucid/misc.h>
#include <lucid/open.h>
#include <lucid/printf.h>

#include "vcc.h"
#include "cmd.h"

void cmd_login(xmlrpc_env *env, int argc, char **argv)
{
	xmlrpc_value *result;
	char *name, *vdir, *cpusetpath;
	char *av[] = { "/bin/sh", NULL };
	int cpusetfd;
	xid_t xid;

	if (argc < 1)
		usage(EXIT_FAILURE);

	name = argv[0];

	result = client_call("vxdb.xid.get",
		"{s:s}",
		"name", name);
	return_if_fault(env);

	xmlrpc_decompose_value(env, result, "i", &xid);
	return_if_fault(env);

	xmlrpc_DECREF(result);

	result = client_call("vxdb.vdir.get",
		"{s:s}",
		"name", name);
	return_if_fault(env);

	xmlrpc_decompose_value(env, result, "s", &vdir);
	return_if_fault(env);

	xmlrpc_DECREF(result);

	/* Add the PID of the current process to the cpuset for this
	 * vserver, if the cpuset exists, and then fork. */
	asprintf(&cpusetpath, "/dev/cpuset/xid%d/tasks", xid);

	if (ispath(cpusetpath)) {
		if ((cpusetfd = open_append(cpusetpath)) == -1) {
			log_perror_and_die("open_append(%s)", cpusetpath);
		}

		else {
			dprintf(cpusetfd, "%d", getpid());
			close(cpusetfd);
		}
	}

	if (ns_enter(xid, 0) == -1)
		log_perror_and_die("ns_enter");

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
