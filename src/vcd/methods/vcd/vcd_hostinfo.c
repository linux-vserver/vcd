// Copyright 2007 Luca Longinotti <chtekk@gentoo.org>
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
#include <dirent.h>

#include "auth.h"
#include "cfg.h"
#include "methods.h"

#include <lucid/log.h>
#include <lucid/scanf.h>
#include <lucid/str.h>

static
int max_mem_node(void)
{
	DIR *ndir;
	struct dirent *dent;
	int maxmemnode = 0;

	ndir = opendir("/sys/devices/system/node");
	if (!ndir)
		return 0;

	while ((dent = readdir(ndir))) {
		if (str_cmpn(dent->d_name, "node", 4))
			continue;

		int node;
		sscanf(dent->d_name+4, "%d", &node);

		if (maxmemnode < node)
			maxmemnode = node;
	}

	closedir(ndir);

	return maxmemnode;
}

/* vcd.hostinfo() */
xmlrpc_value *m_vcd_hostinfo(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	LOG_TRACEME

	xmlrpc_value *response = NULL;

	method_init(env, p, c, VCD_CAP_INFO, 0);
	method_return_if_fault(env);

	response = xmlrpc_build_value(env,
			"{s:s,s:i,s:i}",
			"vbasedir", cfg_getstr(cfg, "vbasedir"),
			"cpus",     sysconf(_SC_NPROCESSORS_ONLN),
			"memnodes", max_mem_node()+1); /* Make both cpus and memnodes here an absolute
			                                * value, not including 0, for consistency */

	return response;
}
