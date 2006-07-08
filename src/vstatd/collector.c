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

#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "lucid.h"
#include "log.h"
#include "cfg.h"
#include "vs_rrd.h"

#define STATSDIR "/proc/virtual"

vstats_limit_t LIMITS[] = {
        { "VM:", 0, 0, 0 },
        { "VML:", 0, 0, 0 },
        { "RSS:", 0, 0, 0 },
        { "ANON:", 0, 0, 0 },
        { "SHM:", 0, 0, 0 },
        { "FILES:", 0, 0, 0 },
        { "OFD:", 0, 0, 0 },
        { "LOCKS:", 0, 0, 0 },
        { "SOCK:", 0, 0, 0 },
        { "MSGQ:", 0, 0, 0 },
        { "SEMA:", 0, 0, 0 },
        { "SEMS:", 0, 0, 0 },
        { "PROC:", 0, 0, 0 },
        { NULL, 0, 0, 0 }
};

vstats_info_t INFO[] = {
        { "nr_threads:", 0 },
        { "nr_running:", 0 },
        { "nr_unintr:", 0 },
        { "nr_onhold:", 0 },
        { NULL, 0 }
};

vstats_loadavg_t LAVG[] = {
        { "loadavg:", 0.00, 0.00, 0.00 },
        { NULL, 0, 0, 0 }
};

vstats_net_t NET[] = {
        { "UNIX:", 0, 0, 0, 0, 0, 0 },
        { "INET:", 0, 0, 0, 0, 0, 0 },
        { "INET6:", 0, 0, 0, 0, 0, 0 },
        { "OTHER:", 0, 0, 0, 0, 0, 0 },
        { NULL, 0, 0, 0, 0, 0, 0 }
};


char *vs_get_vname(xid_t xid, struct vx_vhi_name vhi_name) {
	vhi_name.field = VHIN_CONTEXT;
	
	if (vx_get_vhi_name(xid, &vhi_name) < 0)
		return NULL;
	return strdup(vhi_name.name);
}

void handle_xid(char *path, xid_t xid, char *vname) 
{	
	char *pch, vnm[VHILEN+1], *npath, *datadir;
	int i, j = 0;

	datadir = cfg_getstr(cfg, "datadir");

	/* For util-vserver guests */
	if (vname[0] == '/') {
		pch = strrchr(vname, '/');
		for (i = pch-vname+1;vname[i];i++) {
			vnm[j++] = vname[i];
		}
		vname[j] = '\0';
	}
	/* For vserver-utils guests */
	else
		snprintf(vnm, sizeof(vnm) - 1, vname);

	if (vs_rrd_check(xid, vnm) == -1) {
		log_info("creating databases for vserver '%s', vserver xid '%d'", vnm, xid);

		npath = path_concat(datadir, vnm);
		if (mkdir(npath, 0755) == -1) {
			log_error("mkdir(%s): %s", npath, strerror(errno));
			return;
		} 
		for (i=0; RRD_DB[i].db; i++) {
			if (RRD_DB[i].func_cr(xid, RRD_DB[i].db, npath) < 0)
				return;
		}
	}

	else if (chdir(path) == -1)
		log_error("chdir(%s): %s", path, strerror(errno));
	
	else if (vs_parse_limit(xid) == -1)
		log_error("cannot collect all limit datas, vserver xid '%d'", xid);
	else if (vs_parse_info(xid) == -1)
		log_error("cannot collect all info datas, vserver xid '%d'", xid);
	else if (vs_parse_loadavg(xid) == -1)
		log_error("cannot collect all load average datas, vserver xid '%d'", xid);
	else if (vs_parse_net(xid) == -1)
		log_error("cannot collect all net datas, vserver xid '%d'", xid);
	else {
                npath = path_concat(datadir, vnm);
		for (i=0; RRD_DB[i].db; i++) {
			if (RRD_DB[i].func_up(xid, RRD_DB[i].db, RRD_DB[i].name, npath) < 0)
				break;
		}
	}
	return;
}

void collector_main(void)
{
	DIR *dirp;
	struct dirent *ditp;
	char *path, vname[VHILEN];
	xid_t xid;
	struct vx_vhi_name v_name;
	
	if ((dirp = opendir(STATSDIR)) == NULL)
		log_error_and_die("opendir(%s): %s", STATSDIR, strerror(errno));
	
	while ((ditp = readdir(dirp)) != NULL) {
		path = path_concat(STATSDIR, ditp->d_name);
		
		if (isdir(path)) {
			xid = atoi(ditp->d_name);
			if (vs_get_vname(xid, v_name) != NULL) {
				snprintf(vname, sizeof(vname) - 1, vs_get_vname(xid, v_name));
				handle_xid(path, xid, vname);
			}
			else
				log_error("cannot retrive name for vserver with xid = '%d'", xid);
		}
	}
	
	closedir(dirp);
	return;
}
