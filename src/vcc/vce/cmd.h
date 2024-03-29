// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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

#ifndef _VCE_CMD_H
#define _VCE_CMD_H

void usage(int rc);

void cmd_dx_limit_get       (xmlrpc_env *env, int argc, char **argv);
void cmd_dx_limit_remove    (xmlrpc_env *env, int argc, char **argv);
void cmd_dx_limit_set       (xmlrpc_env *env, int argc, char **argv);
void cmd_init_get           (xmlrpc_env *env, int argc, char **argv);
void cmd_init_set           (xmlrpc_env *env, int argc, char **argv);
void cmd_list               (xmlrpc_env *env, int argc, char **argv);
void cmd_mount_get          (xmlrpc_env *env, int argc, char **argv);
void cmd_mount_remove       (xmlrpc_env *env, int argc, char **argv);
void cmd_mount_set          (xmlrpc_env *env, int argc, char **argv);
void cmd_nx_addr_get        (xmlrpc_env *env, int argc, char **argv);
void cmd_nx_addr_remove     (xmlrpc_env *env, int argc, char **argv);
void cmd_nx_addr_set        (xmlrpc_env *env, int argc, char **argv);
void cmd_nx_broadcast_get   (xmlrpc_env *env, int argc, char **argv);
void cmd_nx_broadcast_remove(xmlrpc_env *env, int argc, char **argv);
void cmd_nx_broadcast_set   (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_bcaps_add       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_bcaps_get       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_bcaps_remove    (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_ccaps_add       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_ccaps_get       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_ccaps_remove    (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_cpuset_get      (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_cpuset_remove   (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_cpuset_set      (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_flags_add       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_flags_get       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_flags_remove    (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_limit_get       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_limit_remove    (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_limit_set       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_sched_get       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_sched_remove    (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_sched_set       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_uname_get       (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_uname_remove    (xmlrpc_env *env, int argc, char **argv);
void cmd_vx_uname_set       (xmlrpc_env *env, int argc, char **argv);

#endif
