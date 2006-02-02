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

#ifndef _COMMANDS_CONTEXT_H_
#define _COMMANDS_CONTEXT_H_

#include <vserver.h>

#include "list.h"

/* prototypes */
int context_create(char *id, char *list);
int context_migrate(char *id);
int context_get(char *id, char *type);
int context_set(char *id, char *type, char *list);
int context_main(int argc, char **argv);

/* system capabilities */
LIST64_START(bcaps_list)
LIST64_NODE(CAP, CHOWN)
LIST64_NODE(CAP, DAC_OVERRIDE)
LIST64_NODE(CAP, DAC_READ_SEARCH)
LIST64_NODE(CAP, FOWNER)
LIST64_NODE(CAP, FSETID)
LIST64_NODE(CAP, FS_MASK)
LIST64_NODE(CAP, KILL)
LIST64_NODE(CAP, SETGID)
LIST64_NODE(CAP, SETUID)
LIST64_NODE(CAP, SETPCAP)
LIST64_NODE(CAP, LINUX_IMMUTABLE)
LIST64_NODE(CAP, NET_BIND_SERVICE)
LIST64_NODE(CAP, NET_BROADCAST)
LIST64_NODE(CAP, NET_ADMIN)
LIST64_NODE(CAP, NET_RAW)
LIST64_NODE(CAP, IPC_LOCK)
LIST64_NODE(CAP, IPC_OWNER)
LIST64_NODE(CAP, SYS_MODULE)
LIST64_NODE(CAP, SYS_RAWIO)
LIST64_NODE(CAP, SYS_CHROOT)
LIST64_NODE(CAP, SYS_PTRACE)
LIST64_NODE(CAP, SYS_PACCT)
LIST64_NODE(CAP, SYS_ADMIN)
LIST64_NODE(CAP, SYS_BOOT)
LIST64_NODE(CAP, SYS_NICE)
LIST64_NODE(CAP, SYS_RESOURCE)
LIST64_NODE(CAP, SYS_TIME)
LIST64_NODE(CAP, SYS_TTY_CONFIG)
LIST64_NODE(CAP, MKNOD)
LIST64_NODE(CAP, LEASE)
LIST64_NODE(CAP, AUDIT_WRITE)
LIST64_NODE(CAP, AUDIT_CONTROL)
LIST64_END

/* context flags */
LIST64_START(cflags_list)
LIST64_NODE(VXF, INFO_LOCK)
LIST64_NODE(VXF, INFO_SCHED)
LIST64_NODE(VXF, INFO_NPROC)
LIST64_NODE(VXF, INFO_PRIVATE)
LIST64_NODE(VXF, INFO_INIT)
LIST64_NODE(VXF, INFO_HIDE)
LIST64_NODE(VXF, INFO_ULIMIT)
LIST64_NODE(VXF, INFO_NSPACE)
LIST64_NODE(VXF, SCHED_HARD)
LIST64_NODE(VXF, SCHED_PRIO)
LIST64_NODE(VXF, SCHED_PAUSE)
LIST64_NODE(VXF, VIRT_MEM)
LIST64_NODE(VXF, VIRT_UPTIME)
LIST64_NODE(VXF, VIRT_CPU)
LIST64_NODE(VXF, VIRT_LOAD)
LIST64_NODE(VXF, HIDE_MOUNT)
LIST64_NODE(VXF, HIDE_NETIF)
LIST64_NODE(VXF, STATE_SETUP)
LIST64_NODE(VXF, STATE_INIT)
LIST64_NODE(VXF, SC_HELPER)
LIST64_NODE(VXF, REBOOT_KILL)
LIST64_NODE(VXF, PERSISTANT)
LIST64_NODE(VXF, FORK_RSS)
LIST64_NODE(VXF, PROLIFIC)
LIST64_END

/* context capabilities */
LIST64_START(ccaps_list)
LIST64_NODE(VXC, CAP_MASK)
LIST64_NODE(VXC, SET_UTSNAME)
LIST64_NODE(VXC, SET_RLIMIT)
LIST64_NODE(VXC, RAW_ICMP)
LIST64_NODE(VXC, SYSLOG)
LIST64_NODE(VXC, SECURE_MOUNT)
LIST64_NODE(VXC, SECURE_REMOUNT)
LIST64_NODE(VXC, BINARY_MOUNT)
LIST64_NODE(VXC, QUOTA_CTL)
LIST64_END

#endif
