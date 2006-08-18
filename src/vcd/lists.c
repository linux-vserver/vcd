// Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
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
#include <vserver.h>
#include <sys/personality.h>

#include "auth.h"
#include "lists.h"

/* system capabilities */
FLIST64_START(bcaps_list)
FLIST64_NODE1(CAP, CHOWN)
FLIST64_NODE1(CAP, DAC_OVERRIDE)
FLIST64_NODE1(CAP, DAC_READ_SEARCH)
FLIST64_NODE1(CAP, FOWNER)
FLIST64_NODE1(CAP, FSETID)
FLIST64_NODE1(CAP, KILL)
FLIST64_NODE1(CAP, SETGID)
FLIST64_NODE1(CAP, SETUID)
FLIST64_NODE1(CAP, SETPCAP)
FLIST64_NODE1(CAP, LINUX_IMMUTABLE)
FLIST64_NODE1(CAP, NET_BIND_SERVICE)
FLIST64_NODE1(CAP, NET_BROADCAST)
FLIST64_NODE1(CAP, NET_ADMIN)
FLIST64_NODE1(CAP, NET_RAW)
FLIST64_NODE1(CAP, IPC_LOCK)
FLIST64_NODE1(CAP, IPC_OWNER)
FLIST64_NODE1(CAP, SYS_MODULE)
FLIST64_NODE1(CAP, SYS_RAWIO)
FLIST64_NODE1(CAP, SYS_CHROOT)
FLIST64_NODE1(CAP, SYS_PTRACE)
FLIST64_NODE1(CAP, SYS_PACCT)
FLIST64_NODE1(CAP, SYS_ADMIN)
FLIST64_NODE1(CAP, SYS_BOOT)
FLIST64_NODE1(CAP, SYS_NICE)
FLIST64_NODE1(CAP, SYS_RESOURCE)
FLIST64_NODE1(CAP, SYS_TIME)
FLIST64_NODE1(CAP, SYS_TTY_CONFIG)
FLIST64_NODE1(CAP, MKNOD)
FLIST64_NODE1(CAP, LEASE)
FLIST64_NODE1(CAP, AUDIT_WRITE)
FLIST64_NODE1(CAP, AUDIT_CONTROL)
FLIST64_END

/* context capabilities */
FLIST64_START(ccaps_list)
//FLIST64_NODE(VXC, CAP_MASK)
FLIST64_NODE(VXC, SET_UTSNAME)
FLIST64_NODE(VXC, SET_RLIMIT)
FLIST64_NODE(VXC, RAW_ICMP)
FLIST64_NODE(VXC, SYSLOG)
FLIST64_NODE(VXC, SECURE_MOUNT)
FLIST64_NODE(VXC, SECURE_REMOUNT)
FLIST64_NODE(VXC, BINARY_MOUNT)
FLIST64_NODE(VXC, QUOTA_CTL)
FLIST64_NODE(VXC, ADMIN_MAPPER)
FLIST64_NODE(VXC, ADMIN_CLOOP)
FLIST64_END

/* context flags */
FLIST64_START(cflags_list)
FLIST64_NODE(VXF, INFO_LOCK)
//FLIST64_NODE(VXF, INFO_SCHED)
//FLIST64_NODE(VXF, INFO_NPROC)
//FLIST64_NODE(VXF, INFO_PRIVATE)
FLIST64_NODE(VXF, INFO_INIT)
FLIST64_NODE(VXF, INFO_HIDE)
//FLIST64_NODE(VXF, INFO_ULIMIT)
//FLIST64_NODE(VXF, INFO_NSPACE)
FLIST64_NODE(VXF, SCHED_HARD)
FLIST64_NODE(VXF, SCHED_PRIO)
FLIST64_NODE(VXF, SCHED_PAUSE)
FLIST64_NODE(VXF, VIRT_MEM)
FLIST64_NODE(VXF, VIRT_UPTIME)
FLIST64_NODE(VXF, VIRT_CPU)
FLIST64_NODE(VXF, VIRT_LOAD)
FLIST64_NODE(VXF, VIRT_TIME)
FLIST64_NODE(VXF, HIDE_MOUNT)
FLIST64_NODE(VXF, HIDE_NETIF)
//FLIST64_NODE(VXF, STATE_SETUP)
//FLIST64_NODE(VXF, STATE_INIT)
//FLIST64_NODE(VXF, STATE_ADMIN)
//FLIST64_NODE(VXF, SC_HELPER)
//FLIST64_NODE(VXF, REBOOT_KILL)
//FLIST64_NODE(VXF, PERSISTENT)
FLIST64_NODE(VXF, FORK_RSS)
//FLIST64_NODE(VXF, PROLIFIC)
FLIST64_END

/* virtual host information (VHI/UTS) types */
FLIST32_START(vhiname_list)
//FLIST32_NODE1(VHIN, CONTEXT)
FLIST32_NODE1(VHIN, SYSNAME)
FLIST32_NODE1(VHIN, NODENAME)
FLIST32_NODE1(VHIN, RELEASE)
FLIST32_NODE1(VHIN, VERSION)
FLIST32_NODE1(VHIN, MACHINE)
FLIST32_NODE1(VHIN, DOMAINNAME)
FLIST32_END

/* resource limits */
FLIST32_START(rlimit_list)
FLIST32_NODE1(RLIMIT, CPU)
FLIST32_NODE1(RLIMIT, FSIZE)
FLIST32_NODE1(RLIMIT, DATA)
FLIST32_NODE1(RLIMIT, STACK)
FLIST32_NODE1(RLIMIT, CORE)
FLIST32_NODE1(RLIMIT, RSS)
FLIST32_NODE1(RLIMIT, NPROC)
FLIST32_NODE1(RLIMIT, NOFILE)
FLIST32_NODE1(RLIMIT, MEMLOCK)
FLIST32_NODE1(RLIMIT, AS)
FLIST32_NODE1(RLIMIT, LOCKS)
FLIST32_NODE1(VLIMIT, NSOCK)
FLIST32_NODE1(VLIMIT, OPENFD)
FLIST32_NODE1(VLIMIT, ANON)
FLIST32_NODE1(VLIMIT, SHMEM)
FLIST32_NODE1(VLIMIT, SEMARY)
FLIST32_NODE1(VLIMIT, NSEMS)
FLIST32_NODE1(VLIMIT, DENTRY)
FLIST32_END

FLIST64_START(vcd_caps_list)
FLIST64_NODE(VCD_CAP, AUTH)
FLIST64_NODE(VCD_CAP, DLIM)
FLIST64_NODE(VCD_CAP, INIT)
FLIST64_NODE(VCD_CAP, MOUNT)
FLIST64_NODE(VCD_CAP, NET)
FLIST64_NODE(VCD_CAP, BCAP)
FLIST64_NODE(VCD_CAP, CCAP)
FLIST64_NODE(VCD_CAP, CFLAG)
FLIST64_NODE(VCD_CAP, RLIM)
FLIST64_NODE(VCD_CAP, SCHED)
FLIST64_NODE(VCD_CAP, UNAME)
FLIST64_NODE(VCD_CAP, CREATE)
FLIST64_NODE(VCD_CAP, EXEC)
FLIST64_NODE(VCD_CAP, INFO)
//FLIST64_NODE(VCD_CAP, HELPER)
FLIST64_END
