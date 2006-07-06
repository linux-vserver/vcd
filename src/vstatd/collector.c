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
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <vserver.h>

#include "log.h"
#include "vs_rrd.h"

int vserver_collector (char *dir, int xid, char *vname) {
    struct vs_limit MIN, MAX, CUR;
    struct vs_net NET;
    struct vs_info INFO;
    int ret = 0;
    
    if ( chdir(dir) < 0) {
       log_error("cannot chdir to directory %s: %s", dir, strerror(errno));
       return -1;
    }   
	
    if (vs_rrd_check(xid) < 0) {
       log_info("creating database '%d.rrd', vserver xid '%d'", xid, xid);
       if (vs_rrd_create(xid) < 0)
	 return -1;   
    } 
   
   if ((ret = vs_init(xid, CUR, MIN, MAX, INFO, NET)) == -1)
    log_error("cannot collect all datas, vserver xid '%d'", xid);
   
   return ret;
}


char *get_vname(xid_t xid) {
    struct vx_vhi_name vhi_name;
   
    vhi_name.field = VHIN_CONTEXT;
    if (vx_get_vhi_name(xid, &vhi_name) >= 0)  
      return vhi_name.name;
    return NULL;
}

char *fpath (char *fp, char *dirname) {
    char dir[PATH_MAX];

    if ((strcmp(fp, ".") == 0) || (strcmp(fp,"..") == 0))
     return NULL;
	
    snprintf(dir, sizeof(dir) - 1, "%s/%s",dirname, fp);
    return dir;
}

int isdir (const char *dir) {
    struct stat statbuf;
    if (lstat(dir, &statbuf) < 0)
     return 0;

    if (S_ISDIR(statbuf.st_mode))
     return 1;			
    
    return 0;
}

int checkdir (char *dirname) {
    DIR *dirp;
    struct dirent *ditp;
    char *fp, vname[VHILEN];
    xid_t xid;
   
    if ((dirp = opendir(dirname)) == NULL)
     log_error_and_die("collector directory %s: %s", dirname, strerror(errno));
   
    while (( ditp = readdir(dirp)) != NULL) {
	if ((fp = fpath(ditp->d_name, dirname)) != NULL) {
	   if ( isdir(fp) ) {
    		xid = atoi(ditp->d_name);  
	        if (get_vname(xid) != NULL) {
		 snprintf(vname,sizeof(vname)-1,get_vname(xid)); 
	         vserver_collector(fp, xid, vname); 	 
		}
                else
                 log_error("cannot get vserver name with xid '%d'", xid);
	   }
   	}
    }
    closedir(dirp);
    return 0;
}
