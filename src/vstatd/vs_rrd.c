#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <rrd.h>

#include "vs_rrd.h"
#include "cfg.h"
#include "log.h"
#include "lucid.h"

char *vs_rrd_build_dbname (xid_t xid) {
    char *dbdir, dbname[ST_BUF - 1];
   
    dbdir = cfg_getstr(cfg, "dbdir");
    snprintf(dbname, sizeof(dbname) - 1, "%s/%d.rrd", dbdir, xid);
   
    return dbname;
}


int vs_rrd_check (xid_t xid) {
    struct stat statinfo;
    char db[ST_BUF];
   
    snprintf(db, sizeof(db) - 1, vs_rrd_build_dbname(xid));
   
    if (lstat(db, &statinfo) < 0)
     return -1;
   
    if (S_ISREG(statinfo.st_mode))
     return 1;   
   
    return -1;
}

int vs_rrd_create (xid_t xid) {
    char db[ST_BUF - 1];
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
       "DS:sys_LOADAVG_OMIN:GAUGE:30:0:9223372036854775807", 
       "DS:sys_LOADAVG_FMIN:GAUGE:30:0:9223372036854775807",
       "DS:sys_LOADAVG_FTMIN:GAUGE:30:0:9223372036854775807",	 
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
    int cargc = sizeof(cargv) / sizeof(*cargv), ret = 0;

    if ((ret = rrd_create(cargc, cargv)) < 0) {
       log_error("cannot create db '%s', vserver xid '%d': %s", db, xid, rrd_get_error());
       rrd_clear_error();
       return -1;
    }
       
    return 0;
}  
   

int vs_rrd_update 
  (xid_t xid, struct vs_limit CUR, struct vs_limit MIN, struct vs_limit MAX, struct vs_info INFO, struct vs_net NET) {
    int cargc = 3, ret = 0;
    char db[ST_BUF], *buf, *cargv[cargc];
    snprintf(db, sizeof(db) - 1, vs_rrd_build_dbname(xid));

    asprintf(&buf, "update %s \
		     N:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d"
		     ":%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d"
		     ":%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d"
		     ":%f:%f:%f:%d:%d:%d:%d"
		     ":%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d"
		     ":%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
	     db, 
	     CUR.mem_VM, CUR.mem_VML, CUR.mem_RSS, CUR.mem_ANON, CUR.mem_SHM, CUR.file_FILES, CUR.file_OFD, 
	     CUR.file_LOCKS, CUR.file_SOCK, CUR.ipc_MSGQ, CUR.ipc_SEMA, CUR.ipc_SEMS, CUR.sys_PROC,
	     MIN.mem_VM, MIN.mem_VML, MIN.mem_RSS, MIN.mem_ANON, MIN.mem_SHM, MIN.file_FILES, MIN.file_OFD, 
	     MIN.file_LOCKS, MIN.file_SOCK, MIN.ipc_MSGQ, MIN.ipc_SEMA, MIN.ipc_SEMS, MIN.sys_PROC,
	     MAX.mem_VM, MAX.mem_VML, MAX.mem_RSS, MAX.mem_ANON, MAX.mem_SHM, MAX.file_FILES, MAX.file_OFD, 
	     MAX.file_LOCKS, MAX.file_SOCK, MAX.ipc_MSGQ, MAX.ipc_SEMA, MAX.ipc_SEMS, MAX.sys_PROC,
             INFO.sys_LOADAVG_1M, INFO.sys_LOADAVG_5M, INFO.sys_LOADAVG_15M, 
	     INFO.thread_TOTAL, INFO.thread_RUN, INFO.thread_NOINT, INFO.thread_HOLD,
             NET.net_UNIX_RECV, NET.net_UNIX_RECV_B, NET.net_UNIX_SEND, 
	     NET.net_UNIX_SEND_B, NET.net_UNIX_FAIL, NET.net_UNIX_FAIL_B,
             NET.net_INET_RECV, NET.net_INET_RECV_B, NET.net_INET_SEND, 
	     NET.net_INET_SEND_B, NET.net_INET_FAIL, NET.net_INET_FAIL_B,
             NET.net_INET6_RECV, NET.net_INET6_RECV_B, NET.net_INET6_SEND, 
	     NET.net_INET6_SEND_B, NET.net_INET6_FAIL, NET.net_INET6_FAIL_B,
             NET.net_OTHER_RECV, NET.net_OTHER_RECV_B, NET.net_OTHER_SEND, 
	     NET.net_OTHER_SEND_B, NET.net_OTHER_FAIL, NET.net_OTHER_FAIL_B);
   
    argv_from_str(buf, cargv, cargc+1);
    
    if ((ret = rrd_update(cargc, cargv)) < 0) {
       log_error("cannot update db '%s', vserver xid '%d': %s", db, xid, rrd_get_error());
       rrd_clear_error();
       return -1;
     }
	
     return 0;
}
   
