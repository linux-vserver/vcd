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
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <vserver.h>

#include "lucid.h"

#include "log.h"
#include "vs_rrd.h"

#define STATSDIR "/proc/virtual"

void handle_xid(char *path, xid_t xid)
{
	struct vs_limit MIN, MAX, CUR;
	struct vs_net NET;
	struct vs_info INFO;
	
	if (chdir(path) == -1)
		log_error("chdir(%s): %s", path, strerror(errno));
	
	else if (vs_rrd_check(xid) < 0) {
		log_info("creating database '%d.rrd', vserver xid '%d'", xid, xid);
		
		if (vs_rrd_create(xid) < 0) {
			log_error("cannot create database");
			return;
		}
	}
	
	else if (vs_init(xid, CUR, MIN, MAX, INFO, NET) == -1)
		log_error("cannot collect all datas, vserver xid '%d'", xid);
}

void collector_main(void)
{
	DIR *dirp;
	struct dirent *ditp;
	char *path;
	xid_t xid;
	
	if ((dirp = opendir(STATSDIR)) == NULL)
		log_error_and_die("opendir(%s): %s", STATSDIR, strerror(errno));
	
	while ((ditp = readdir(dirp)) != NULL) {
		if ((path = path_concat(STATSDIR, ditp->d_name)) == NULL)
			log_error_and_die("path_concat: %s", strerror(errno));
		
		if (isdir(path)) {
			xid = atoi(ditp->d_name);
			handle_xid(path, xid);
		}
	}
	
	closedir(dirp);
}
