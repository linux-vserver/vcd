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

#ifndef _VSTATD_VSTATS_H
#define _VSTATD_VSTATS_H
#include <vserver.h>

#define ST_BUF 512

#define VS_LIM_VL 3
#define VS_LAVG_VL 3
#define VS_INFO_VL 1
#define VS_NET_VL 6
#define VS_ALL 70

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

int vs_init (xid_t xid, struct vs_limit CUR, struct vs_limit MIN, struct vs_limit MAX, struct vs_info INFO, struct vs_net NET);

#endif
