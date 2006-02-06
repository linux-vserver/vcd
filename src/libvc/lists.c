/***************************************************************************
 *   Copyright 2005-2006 by the vserver-utils team                         *
 *   See AUTHORS for details                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "vc.h"

/* system capabilities */
VC_LIST64_START(vc_bcaps_list)
VC_LIST64_NODE(CAP, CHOWN)
VC_LIST64_NODE(CAP, DAC_OVERRIDE)
VC_LIST64_NODE(CAP, DAC_READ_SEARCH)
VC_LIST64_NODE(CAP, FOWNER)
VC_LIST64_NODE(CAP, FSETID)
VC_LIST64_NODE(CAP, FS_MASK)
VC_LIST64_NODE(CAP, KILL)
VC_LIST64_NODE(CAP, SETGID)
VC_LIST64_NODE(CAP, SETUID)
VC_LIST64_NODE(CAP, SETPCAP)
VC_LIST64_NODE(CAP, LINUX_IMMUTABLE)
VC_LIST64_NODE(CAP, NET_BIND_SERVICE)
VC_LIST64_NODE(CAP, NET_BROADCAST)
VC_LIST64_NODE(CAP, NET_ADMIN)
VC_LIST64_NODE(CAP, NET_RAW)
VC_LIST64_NODE(CAP, IPC_LOCK)
VC_LIST64_NODE(CAP, IPC_OWNER)
VC_LIST64_NODE(CAP, SYS_MODULE)
VC_LIST64_NODE(CAP, SYS_RAWIO)
VC_LIST64_NODE(CAP, SYS_CHROOT)
VC_LIST64_NODE(CAP, SYS_PTRACE)
VC_LIST64_NODE(CAP, SYS_PACCT)
VC_LIST64_NODE(CAP, SYS_ADMIN)
VC_LIST64_NODE(CAP, SYS_BOOT)
VC_LIST64_NODE(CAP, SYS_NICE)
VC_LIST64_NODE(CAP, SYS_RESOURCE)
VC_LIST64_NODE(CAP, SYS_TIME)
VC_LIST64_NODE(CAP, SYS_TTY_CONFIG)
VC_LIST64_NODE(CAP, MKNOD)
VC_LIST64_NODE(CAP, LEASE)
VC_LIST64_NODE(CAP, AUDIT_WRITE)
VC_LIST64_NODE(CAP, AUDIT_CONTROL)
VC_LIST64_END

/* context capabilities */
VC_LIST64_START(vc_ccaps_list)
VC_LIST64_NODE(VXC, CAP_MASK)
VC_LIST64_NODE(VXC, SET_UTSNAME)
VC_LIST64_NODE(VXC, SET_RLIMIT)
VC_LIST64_NODE(VXC, RAW_ICMP)
VC_LIST64_NODE(VXC, SYSLOG)
VC_LIST64_NODE(VXC, SECURE_MOUNT)
VC_LIST64_NODE(VXC, SECURE_REMOUNT)
VC_LIST64_NODE(VXC, BINARY_MOUNT)
VC_LIST64_NODE(VXC, QUOTA_CTL)
VC_LIST64_END

/* context flags */
VC_LIST64_START(vc_cflags_list)
VC_LIST64_NODE(VXF, INFO_LOCK)
VC_LIST64_NODE(VXF, INFO_SCHED)
VC_LIST64_NODE(VXF, INFO_NPROC)
VC_LIST64_NODE(VXF, INFO_PRIVATE)
VC_LIST64_NODE(VXF, INFO_INIT)
VC_LIST64_NODE(VXF, INFO_HIDE)
VC_LIST64_NODE(VXF, INFO_ULIMIT)
VC_LIST64_NODE(VXF, INFO_NSPACE)
VC_LIST64_NODE(VXF, SCHED_HARD)
VC_LIST64_NODE(VXF, SCHED_PRIO)
VC_LIST64_NODE(VXF, SCHED_PAUSE)
VC_LIST64_NODE(VXF, VIRT_MEM)
VC_LIST64_NODE(VXF, VIRT_UPTIME)
VC_LIST64_NODE(VXF, VIRT_CPU)
VC_LIST64_NODE(VXF, VIRT_LOAD)
VC_LIST64_NODE(VXF, HIDE_MOUNT)
VC_LIST64_NODE(VXF, HIDE_NETIF)
VC_LIST64_NODE(VXF, STATE_SETUP)
VC_LIST64_NODE(VXF, STATE_INIT)
VC_LIST64_NODE(VXF, SC_HELPER)
VC_LIST64_NODE(VXF, REBOOT_KILL)
VC_LIST64_NODE(VXF, PERSISTANT)
VC_LIST64_NODE(VXF, FORK_RSS)
VC_LIST64_NODE(VXF, PROLIFIC)
VC_LIST64_END

/* virtual host information (VHI/UTS) types */
VC_LIST32_START(vc_vhiname_list)
VC_LIST32_NODE(VHIN, CONTEXT)
VC_LIST32_NODE(VHIN, SYSNAME)
VC_LIST32_NODE(VHIN, NODENAME)
VC_LIST32_NODE(VHIN, RELEASE)
VC_LIST32_NODE(VHIN, VERSION)
VC_LIST32_NODE(VHIN, MACHINE)
VC_LIST32_NODE(VHIN, DOMAINNAME)
VC_LIST32_END

/* inode attributes */
VC_LIST32_START(vc_iattr_list)
VC_LIST32_NODE(IATTR, XID)
VC_LIST32_NODE(IATTR, ADMIN)
VC_LIST32_NODE(IATTR, WATCH)
VC_LIST32_NODE(IATTR, HIDE)
VC_LIST32_NODE(IATTR, FLAGS)
VC_LIST32_NODE(IATTR, BARRIER)
VC_LIST32_NODE(IATTR, IUNLINK)
VC_LIST32_NODE(IATTR, IMMUTABLE)
VC_LIST32_END

/* resource limits */
VC_LIST32_START(vc_rlimit_list)
VC_LIST32_NODE(RLIMIT, CPU)
VC_LIST32_NODE(RLIMIT, FSIZE)
VC_LIST32_NODE(RLIMIT, DATA)
VC_LIST32_NODE(RLIMIT, STACK)
VC_LIST32_NODE(RLIMIT, CORE)
VC_LIST32_NODE(RLIMIT, RSS)
VC_LIST32_NODE(RLIMIT, NPROC)
VC_LIST32_NODE(RLIMIT, NOFILE)
VC_LIST32_NODE(RLIMIT, MEMLOCK)
VC_LIST32_NODE(RLIMIT, AS)
VC_LIST32_NODE(RLIMIT, LOCKS)
VC_LIST32_NODE(VLIMIT, NSOCK)
VC_LIST32_NODE(VLIMIT, OPENFD)
VC_LIST32_NODE(VLIMIT, ANON)
VC_LIST32_NODE(VLIMIT, SHMEM)
VC_LIST32_NODE(VLIMIT, SEMARY)
VC_LIST32_NODE(VLIMIT, NSEMS)
VC_LIST32_END

/* network context flags */
VC_LIST64_START(vc_nflags_list)
VC_LIST64_NODE(NXF, STATE_SETUP)
VC_LIST64_NODE(NXF, SC_HELPER)
VC_LIST64_NODE(NXF, PERSISTANT)
VC_LIST64_END

/* scheduler option list */
VC_LIST32_START(vc_sched_list)
VC_LIST32_NODE(VXSM, FILL_RATE)
VC_LIST32_NODE(VXSM, INTERVAL)
VC_LIST32_NODE(VXSM, TOKENS)
VC_LIST32_NODE(VXSM, TOKENS_MIN)
VC_LIST32_NODE(VXSM, TOKENS_MAX)
VC_LIST32_NODE(VXSM, PRIO_BIAS)
VC_LIST32_END
