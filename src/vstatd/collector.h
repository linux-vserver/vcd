#ifndef _VSTATD_COLLECTOR_H
#define _VSTATD_COLLECTOR_H

struct vs_limit {   
       int mem_VM;
       int mem_VML;
       int mem_RSS;
       int mem_ANON;
       int mem_SHM;
       int file_FILES;
       int file_OFD;
       int file_LOCKS;
       int file_SOCK;
       int ipc_MSGQ;
       int ipc_SEMA;
       int ipc_SEMS;
       int sys_PROC;
};

struct vs_info {   
       float sys_LOADAVG_1M;
       float sys_LOADAVG_5M;
       float sys_LOADAVG_15M;
       int thread_TOTAL;
       int thread_RUN;
       int thread_NOINT;
       int thread_HOLD;
};

struct vs_net {   
       int net_UNIX_RECV;
       int net_UNIX_RECV_B;
       int net_UNIX_SEND;
       int net_UNIX_SEND_B;
       int net_UNIX_FAIL;
       int net_UNIX_FAIL_B;
       int net_INET_RECV;
       int net_INET_RECV_B;
       int net_INET_SEND;
       int net_INET_SEND_B;
       int net_INET_FAIL;
       int net_INET_FAIL_B;
       int net_INET6_RECV;
       int net_INET6_RECV_B;
       int net_INET6_SEND;
       int net_INET6_SEND_B;
       int net_INET6_FAIL;
       int net_INET6_FAIL_B;
       int net_OTHER_RECV;
       int net_OTHER_RECV_B;
       int net_OTHER_SEND;
       int net_OTHER_SEND_B;
       int net_OTHER_FAIL;
       int net_OTHER_FAIL_B;
};

#endif
