
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <rrd.h>
#include <stdarg.h>

#include "vs_rrd.h"
#include "log.h"

char *VS_FILES[] = {   
    "limit",
    "cvirt",
    "cacct",
};

struct vstats { char *name;  int idt; } vnms[] = {
     { "VM:", 0 },
     { "VML:", 1 },
     { "RSS:", 2 },
     { "ANON:", 3 },
     { "SHM:", 4 },
     { "FILES:", 5 },
     { "OFD:", 6 },
     { "LOCKS:", 7 },
     { "SOCK:", 8 },
     { "MSGQ:", 9 },
     { "SEMA:", 10 },
     { "SEMS:", 11 },
     { "PROC:", 12 },

     { "loadavg:", 13 },
     { "nr_threads:", 14 },
     { "nr_running:", 15 },
     { "nr_unintr:", 16 },
     { "nr_onhold:", 17 },

     { "UNIX:", 18 },
     { "INET:", 19 },
     { "INET6:", 20 },
     { "OTHER:", 21 },
};


int snscanf(const char* buf, int len, const char* format, ...) {   
    va_list vargs;
    int ret;
    char *sbuf = (char *) malloc (len + 1);
   
    snprintf(sbuf,len, buf);
    
    va_start(vargs, format);
    ret = vsscanf(sbuf, format, vargs);
    va_end(vargs);
    return ret;
}



int vstats_getname (char *buf) {
    char pr[ST_BUF + 2];
    int ln = sizeof(vnms) / sizeof(*vnms), i;

    snscanf(buf, ST_BUF, "%s", pr);
    for (i=0; i < ln; i++) {
       if ( strcmp(vnms[i].name, pr) == 0)
        return vnms[i].idt;
    }
   return -1;
}

int  vs_init (xid_t xid, struct vs_limit CUR, struct vs_limit MIN, struct vs_limit MAX, struct vs_info INFO, struct vs_net NET) {
    FILE *vfp;
    char rd[4096], *fp;
    int nm, ret, i, ct = sizeof(VS_FILES) / sizeof(*VS_FILES);
          
    for (i=0;i<ct;i++) {
       fp = VS_FILES[i]; 
       if ((vfp = fopen(fp, "r")) != NULL) {
	  while (fgets(rd, sizeof(rd) - 1, vfp) != NULL) {
	     nm = vstats_getname(rd);
	     switch ( nm ) { 
	      case 0:
	       if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.mem_VM, &MIN.mem_VM, &MAX.mem_VM)) != VS_LIM_VL)
	        return -1;
	       break;
	      case 1:
	       if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.mem_VML, &MIN.mem_VML, &MAX.mem_VML)) != VS_LIM_VL)
	        return -1;
	       break;
              case 2:
	       if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.mem_RSS, &MIN.mem_RSS, &MAX.mem_RSS)) != VS_LIM_VL)
	        return -1;
	       break;
              case 3:		  
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.mem_ANON, &MIN.mem_ANON, &MAX.mem_ANON)) != VS_LIM_VL)
	        return -1;
               break;
              case 4:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.mem_SHM, &MIN.mem_SHM, &MAX.mem_SHM)) != VS_LIM_VL)
                return -1;
	       break;
              case 5:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.file_FILES, &MIN.file_FILES, &MAX.file_FILES)) != VS_LIM_VL)
                return -1;
	       break;
              case 6:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.file_OFD, &MIN.file_OFD, &MAX.file_OFD)) != VS_LIM_VL)
                return -1;
	       break;
              case 7:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.file_LOCKS, &MIN.file_LOCKS, &MAX.file_LOCKS)) != VS_LIM_VL)
                return -1;
	       break;
              case 8:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.file_SOCK, &MIN.file_SOCK, &MAX.file_SOCK)) != VS_LIM_VL)
                return -1;
	       break;
              case 9:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.ipc_MSGQ, &MIN.ipc_MSGQ, &MAX.ipc_MSGQ)) != VS_LIM_VL)
                return -1;
	       break;
              case 10:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.ipc_SEMA, &MIN.ipc_SEMA, &MAX.ipc_SEMA)) != VS_LIM_VL)
                return -1;
	       break;
              case 11:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.ipc_SEMS, &MIN.ipc_SEMS, &MAX.ipc_SEMS)) != VS_LIM_VL)
                return -1;
	       break;
              case 12:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %d %*c %d", &CUR.sys_PROC, &MIN.sys_PROC, &MAX.sys_PROC)) != VS_LIM_VL)
                return -1;
	       break;
              case 13:
               if ((ret = snscanf(rd, ST_BUF, "%*s %f %f %f", &INFO.sys_LOADAVG_1M, &INFO.sys_LOADAVG_5M, &INFO.sys_LOADAVG_15M)) != VS_LAVG_VL)
	        return -1;
               break;
              case 14:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d", &INFO.thread_TOTAL)) != VS_INFO_VL)
	        return -1;
               break;
              case 15:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d", &INFO.thread_RUN)) != VS_INFO_VL)
	        return -1;
               break;
              case 16:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d", &INFO.thread_NOINT)) != VS_INFO_VL)
	        return -1;
               break;
              case 17:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d", &INFO.thread_HOLD)) != VS_INFO_VL)
	        return -1;
               break;
              case 18:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %*c %d %d %*c %d %d %*c %d", &NET.net_UNIX_RECV, &NET.net_UNIX_RECV_B, &NET.net_UNIX_SEND, &NET.net_UNIX_SEND_B, &NET.net_UNIX_FAIL, &NET.net_UNIX_FAIL_B)) != VS_NET_VL)
	        return -1;
 	       break;
              case 19:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %*c %d %d %*c %d %d %*c %d", &NET.net_INET_RECV, &NET.net_INET_RECV_B, &NET.net_INET_SEND, &NET.net_INET_SEND_B, &NET.net_INET_FAIL, &NET.net_INET_FAIL_B)) != VS_NET_VL)
	        return -1;  
               break;
   	      case 20:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %*c %d %d %*c %d %d %*c %d", &NET.net_INET6_RECV, &NET.net_INET6_RECV_B, &NET.net_INET6_SEND, &NET.net_INET6_SEND_B, &NET.net_INET6_FAIL, &NET.net_INET6_FAIL_B)) != VS_NET_VL)
	        return -1;
               break;
              case 21:
               if ((ret = snscanf(rd, ST_BUF, "%*s %d %*c %d %d %*c %d %d %*c %d", &NET.net_OTHER_RECV, &NET.net_OTHER_RECV_B, &NET.net_OTHER_SEND, &NET.net_OTHER_SEND_B, &NET.net_OTHER_FAIL, &NET.net_OTHER_FAIL_B)) != VS_NET_VL)
	        return -1;
               break;
              default:
               break;
	     }
	  }
       fclose(vfp);
       }
       else
        log_error("vserver xid '%d', filename '%s': %s", xid, fp, strerror(errno)); 
    }
   
    if (vs_rrd_update(xid, CUR, MIN, MAX, INFO, NET) < 0)
     return -2;
    return 0;
}

   
