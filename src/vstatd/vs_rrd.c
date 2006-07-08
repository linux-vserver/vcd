// Copyright 2006 Remo Lemma <coloss7@gmail.com>
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "vs_rrd.h"
#include "cfg.h"
#include "log.h"
#include "lucid.h"


vs_rrd_db RRD_DB[] = {
	{ "mem_VM.rrd", vs_rrd_create_limit },
	{ "mem_VML.rrd", vs_rrd_create_limit },
	{ "mem_RSS.rrd", vs_rrd_create_limit },
	{ "mem_ANON.rrd", vs_rrd_create_limit },
	{ "mem_SHM.rrd", vs_rrd_create_limit },
	{ "file_FILES.rrd", vs_rrd_create_limit },
	{ "file_OFD.rrd", vs_rrd_create_limit },
	{ "file_LOCKS.rrd", vs_rrd_create_limit },
	{ "file_SOCK.rrd", vs_rrd_create_limit },
	{ "ipc_MSGQ.rrd", vs_rrd_create_limit },
	{ "ipc_SEMA.rrd", vs_rrd_create_limit },
	{ "ipc_SEMS.rrd", vs_rrd_create_limit },
	{ "sys_PROC.rrd", vs_rrd_create_limit },
	{ "sys_LOADAVG.rrd", vs_rrd_create_loadavg },
	{ "thread_TOTAL.rrd", vs_rrd_create_info },
	{ "thread_RUN.rrd", vs_rrd_create_info },
	{ "thread_NOINT.rrd", vs_rrd_create_info },
	{ "thread_HOLD.rrd", vs_rrd_create_info },
	{ "net_UNIX.rrd", vs_rrd_create_net },
	{ "net_INET.rrd", vs_rrd_create_net },
	{ "net_INET6.rrd", vs_rrd_create_net },
	{ "net_OTHER.rrd", vs_rrd_create_net },
	{ NULL, NULL }
};


int vs_rrd_check (xid_t xid, char *vname) 
{
	char *datadir = NULL, *path;
	int i;

	datadir = cfg_getstr(cfg, "datadir");    
	path = path_concat(datadir, vname);

	if (isdir(path)) {
		if (chdir(path) == -1) {
			log_error("no update will be done -> chdir(%s): %s", path, strerror(errno));
			return -2;
		}
		for (i=0; RRD_DB[i].db; i++) {
			if (isfile(RRD_DB[i].db) == 0) {
				log_info("creating database '%s' for vserver '%s', xid '%d'", RRD_DB[i].db, vname, xid);
				RRD_DB[i].func(xid, RRD_DB[i].db, path);
			}
		}
	}
	else
		return -1;
	return 0;
}
