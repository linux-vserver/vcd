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
	{ "mem_VM.rrd", "VM:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "mem_VML.rrd", "VML:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "mem_RSS.rrd", "RSS:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "mem_ANON.rrd", "ANON:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "mem_SHM.rrd", "SHM:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "file_FILES.rrd", "FILES:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "file_OFD.rrd", "OFD:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "file_LOCKS.rrd", "LOCKS:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "file_SOCK.rrd", "SOCK:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "ipc_MSGQ.rrd", "MSGQ:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "ipc_SEMA.rrd", "SEMA:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "ipc_SEMS.rrd", "SEMS:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "sys_PROC.rrd", "PROC:", vs_rrd_create_limit, vs_rrd_update_limit },
	{ "sys_LOADAVG.rrd", "loadavg:", vs_rrd_create_loadavg, vs_rrd_update_loadavg },
	{ "thread_TOTAL.rrd", "nr_threads:", vs_rrd_create_info, vs_rrd_update_info },
	{ "thread_RUN.rrd", "nr_running:", vs_rrd_create_info, vs_rrd_update_info },
	{ "thread_NOINT.rrd", "nr_unintr:", vs_rrd_create_info, vs_rrd_update_info },
	{ "thread_HOLD.rrd", "nr_onhold:", vs_rrd_create_info, vs_rrd_update_info },
	{ "net_UNIX.rrd", "UNIX:", vs_rrd_create_net, vs_rrd_update_net },
	{ "net_INET.rrd", "INET:", vs_rrd_create_net, vs_rrd_update_net },
	{ "net_INET6.rrd", "INET6:", vs_rrd_create_net, vs_rrd_update_net },
	{ "net_OTHER.rrd", "OTHER:", vs_rrd_create_net, vs_rrd_update_net },
	{ NULL, NULL, NULL, NULL }
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
				RRD_DB[i].func_cr(xid, RRD_DB[i].db, path);
			}
		}
	}
	else
		return -1;
	return 0;
}
