#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <rrd.h>

#include "vs_rrd.h"
#include "cfg.h"
#include "log.h"

char *vs_rrd_build_dbname (xid_t xid) {
    char *dbdir, dbname[127];
   
    dbdir = cfg_getstr(cfg, "dbdir");
    snprintf(dbname, sizeof(dbname) - 1, "%s/%d.rrd", dbdir, xid);
   
    return dbname;
}

int vs_rrd_check (xid_t xid) {
    struct stat statinfo;
    char db[128];
   
    snprintf(db, sizeof(db) - 1, vs_rrd_build_dbname(xid));
   
    if (lstat(db, &statinfo) < 0)
     return -1;
   
    if (S_ISREG(statinfo.st_mode))
     return 1;   
   
    return -1;
}


int vs_rrd_create (xid_t xid) {
    char db[ST_BUF];
    snprintf(db, sizeof(db) - 1, vs_rrd_build_dbname(xid));
    
    char *cargv[] = {
       "create",
       db,
       "-s 30",
       "DS:mem_VM:GAUGE:30:0:9223372036854775807",
       "DS:mem_VML:GAUGE:30:0:9223372036854775807", 
       "DS:mem_RSS:GAUGE:30:0:9223372036854775807", 
       "DS:mem_ANON:GAUGE:30:0:9223372036854775807", 
       "DS:mem_SHM:GAUGE:30:0:9223372036854775807", 
       "DS:file_FILES:GAUGE:30:0:9223372036854775807", 
       "DS:file_OFD:GAUGE:30:0:9223372036854775807", 
       "DS:file_LOCKS:GAUGE:30:0:9223372036854775807", 
       "DS:file_SOCK:GAUGE:30:0:9223372036854775807", 
       "DS:ipc_MSGQ:GAUGE:30:0:9223372036854775807", 
       "DS:ipc_SEMA:GAUGE:30:0:9223372036854775807", 
       "DS:ipc_SEMS:GAUGE:30:0:9223372036854775807", 
       "DS:sys_PROC:GAUGE:30:0:9223372036854775807", 

       "DS:mem_VM_MIN:GAUGE:30:0:9223372036854775807",
       "DS:mem_VML_MIN:GAUGE:30:0:9223372036854775807",
       "DS:mem_RSS_MIN:GAUGE:30:0:9223372036854775807",
       "DS:mem_ANON_MIN:GAUGE:30:0:9223372036854775807",
       "DS:mem_SHM_MIN:GAUGE:30:0:9223372036854775807",
       "DS:file_FILES_MIN:GAUGE:30:0:9223372036854775807",
       "DS:file_OFD_MIN:GAUGE:30:0:9223372036854775807",
       "DS:file_LOCKS_MIN:GAUGE:30:0:9223372036854775807",
       "DS:file_SOCK_MIN:GAUGE:30:0:9223372036854775807",
       "DS:ipc_MSGQ_MIN:GAUGE:30:0:9223372036854775807",
       "DS:ipc_SEMA_MIN:GAUGE:30:0:9223372036854775807",
       "DS:ipc_SEMS_MIN:GAUGE:30:0:9223372036854775807",
       "DS:sys_PROC_MIN:GAUGE:30:0:9223372036854775807",

       "DS:mem_VM_MAX:GAUGE:30:0:9223372036854775807",
       "DS:mem_VML_MAX:GAUGE:30:0:9223372036854775807",
       "DS:mem_RSS_MAX:GAUGE:30:0:9223372036854775807",
       "DS:mem_ANON_MAX:GAUGE:30:0:9223372036854775807",
       "DS:mem_SHM_MAX:GAUGE:30:0:9223372036854775807",
       "DS:file_FILES_MAX:GAUGE:30:0:9223372036854775807",
       "DS:file_OFD_MAX:GAUGE:30:0:9223372036854775807",
       "DS:file_LOCKS_MAX:GAUGE:30:0:9223372036854775807",
       "DS:file_SOCK_MAX:GAUGE:30:0:9223372036854775807",
       "DS:ipc_MSGQ_MAX:GAUGE:30:0:9223372036854775807",
       "DS:ipc_SEMA_MAX:GAUGE:30:0:9223372036854775807",
       "DS:ipc_SEMS_MAX:GAUGE:30:0:9223372036854775807",
       "DS:sys_PROC_MAX:GAUGE:30:0:9223372036854775807",

	 
       "DS:sys_LOADAVG:GAUGE:30:0:9223372036854775807", 

	 
       "DS:thread_TOTAL:GAUGE:30:0:9223372036854775807", 
       "DS:thread_RUN:GAUGE:30:0:9223372036854775807",
       "DS:thread_NOINT:GAUGE:30:0:9223372036854775807", 
       "DS:thread_HOLD:GAUGE:30:0:9223372036854775807", 
       
	 
       "DS:net_UNIX_RECV:GAUGE:30:0:9223372036854775807",
       "DS:net_UNIX_RECV_B:GAUGE:30:0:9223372036854775807",
       "DS:net_UNIX_SEND:GAUGE:30:0:9223372036854775807", 
       "DS:net_UNIX_SEND_B:GAUGE:30:0:9223372036854775807",
       "DS:net_UNIX_FAIL:GAUGE:30:0:9223372036854775807", 
       "DS:net_UNIX_FAIL_B:GAUGE:30:0:9223372036854775807", 
       
       "DS:net_INET_RECV:GAUGE:30:0:9223372036854775807",
       "DS:net_INET_RECV_B:GAUGE:30:0:9223372036854775807",
       "DS:net_INET_SEND:GAUGE:30:0:9223372036854775807",
       "DS:net_INET_SEND_B:GAUGE:30:0:9223372036854775807", 
       "DS:net_INET_FAIL:GAUGE:30:0:9223372036854775807", 
       "DS:net_INET_FAIL_B:GAUGE:30:0:9223372036854775807", 
     
       "DS:net_INET6_RECV:GAUGE:30:0:9223372036854775807",
       "DS:net_INET6_RECV_B:GAUGE:30:0:9223372036854775807",
       "DS:net_INET6_SEND:GAUGE:30:0:9223372036854775807", 
       "DS:net_INET6_SEND_B:GAUGE:30:0:9223372036854775807",
       "DS:net_INET6_FAIL:GAUGE:30:0:9223372036854775807", 
       "DS:net_INET6_FAIL_B:GAUGE:30:0:9223372036854775807", 
       
       "DS:net_OTHER_RECV:GAUGE:30:0:9223372036854775807",
       "DS:net_OTHER_RECV_B:GAUGE:30:0:9223372036854775807",
       "DS:net_OTHER_SEND:GAUGE:30:0:9223372036854775807",
       "DS:net_OTHER_SEND_B:GAUGE:30:0:9223372036854775807", 
       "DS:net_OTHER_FAIL:GAUGE:30:0:9223372036854775807", 
       "DS:net_OTHER_FAIL_B:GAUGE:30:0:9223372036854775807",
       
       "RRA:MIN:0:1:60",
       "RRA:MAX:0:1:60", 
       "RRA:AVERAGE:0:1:60", 
       "RRA:MIN:0:12:60",
       "RRA:MAX:0:12:60", 
       "RRA:AVERAGE:0:12:60", 
       "RRA:MIN:0:48:60", 
       "RRA:MAX:0:48:60", 
       "RRA:AVERAGE:0:48:60", 
       "RRA:MIN:0:1440:60",
       "RRA:MAX:0:1440:60", 
       "RRA:AVERAGE:0:1440:60",
       "RRA:MIN:0:17520:60",
       "RRA:MAX:0:17520:60", 
       "RRA:AVERAGE:0:17520:60",
    };
    int cargc = sizeof(cargv) / sizeof(*cargv), ret;
   
    if ((ret = rrd_create(cargc, cargv))) {
       log_error("cannot create '%s' for vserver with xid '%d'", db, xid);
       return -1;
    }
   	
    return 0;
}

int vs_rrd_update (xid_t xid, struct vs_limit CUR, struct vs_limit MIN, struct vs_limit MAX, struct vs_info INFO, struct vs_net NET) {
    return 0;
}
   
