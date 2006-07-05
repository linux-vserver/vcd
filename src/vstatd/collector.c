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

    if ( chdir(dir) < 0) {
       log_error("cannot chdir to directory %s: %s", dir, strerror(errno));
       return -1;
    }   
	
    if (vs_rrd_check(xid) < 0) {
       log_info("creating database for vserver with xid '%d'", xid);
       if (vs_rrd_create(xid) < 0)
	 return -1;   
    } 
   
    if (vstats_init (LIMIT_FILE, xid, MIN, MAX, CUR, NET, INFO) < 0) {
       log_error("vserver xid: '%d' -> cannot collect all the Limit datas", xid);
       return -1;
    }	  
	
    if (vstats_init (INFO_FILE, xid, MIN, MAX, CUR, NET, INFO) < 0) {
       log_error("vserver xid '%d' -> cannot collect all the Info datas", xid);
       return -1;
    }
   
    if (vstats_init (NET_FILE, xid, MIN, MAX, CUR, NET, INFO) < 0) {
       log_error("vserver with xid '%d' -> cannot collect all the Net datas", xid);
       return -1;
    }	

    if (vs_rrd_update (xid, CUR, MIN, MAX, INFO, NET) < 0)
     return -1;
    
    return 0;
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
     log_error_and_die("collector directory: %s", strerror(errno));
   
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
    exit(EXIT_SUCCESS);
}
