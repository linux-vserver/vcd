## Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the
## Free Software Foundation, Inc.,
## 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

INCLUDES = -I$(top_srcdir)/src/vcd

M_HELPER = helper/helper_netup.c \
           helper/helper_restart.c \
           helper/helper_shutdown.c \
           helper/helper_startup.c

M_VCD = vcd/vcd_login.c \
        vcd/vcd_status.c \
        vcd/vcd_user_caps_add.c \
        vcd/vcd_user_caps_get.c \
        vcd/vcd_user_caps_remove.c \
        vcd/vcd_user_get.c \
        vcd/vcd_user_remove.c \
        vcd/vcd_user_set.c

M_VG = vg/vg_add.c \
       vg/vg_list.c \
       vg/vg_remove.c \
       vg/vg_vx_start.c \
       vg/vg_vx_stop.c

M_VX = vx/vx_create.c \
       vx/vx_exec.c \
       vx/vx_kill.c \
       vx/vx_load.c \
       vx/vx_reboot.c \
       vx/vx_remove.c \
       vx/vx_rename.c \
       vx/vx_restart.c \
       vx/vx_start.c \
       vx/vx_status.c \
       vx/vx_stop.c

M_VXDB = vxdb/vxdb_dep_add.c \
         vxdb/vxdb_dep_remove.c \
         vxdb/vxdb_dx_limit_get.c \
         vxdb/vxdb_dx_limit_remove.c \
         vxdb/vxdb_dx_limit_set.c \
         vxdb/vxdb_init_get.c \
         vxdb/vxdb_init_set.c \
         vxdb/vxdb_list.c \
         vxdb/vxdb_mount_get.c \
         vxdb/vxdb_mount_remove.c \
         vxdb/vxdb_mount_set.c \
         vxdb/vxdb_name_get.c \
         vxdb/vxdb_nx_addr_get.c \
         vxdb/vxdb_nx_addr_remove.c \
         vxdb/vxdb_nx_addr_set.c \
         vxdb/vxdb_nx_broadcast_get.c \
         vxdb/vxdb_nx_broadcast_remove.c \
         vxdb/vxdb_nx_broadcast_set.c \
         vxdb/vxdb_owner_add.c \
         vxdb/vxdb_owner_get.c \
         vxdb/vxdb_owner_remove.c \
         vxdb/vxdb_vdir_get.c \
         vxdb/vxdb_vx_bcaps_add.c \
         vxdb/vxdb_vx_bcaps_get.c \
         vxdb/vxdb_vx_bcaps_remove.c \
         vxdb/vxdb_vx_ccaps_add.c \
         vxdb/vxdb_vx_ccaps_get.c \
         vxdb/vxdb_vx_ccaps_remove.c \
         vxdb/vxdb_vx_flags_add.c \
         vxdb/vxdb_vx_flags_get.c \
         vxdb/vxdb_vx_flags_remove.c \
         vxdb/vxdb_vx_limit_get.c \
         vxdb/vxdb_vx_limit_remove.c \
         vxdb/vxdb_vx_limit_set.c \
         vxdb/vxdb_vx_sched_get.c \
         vxdb/vxdb_vx_sched_remove.c \
         vxdb/vxdb_vx_sched_set.c \
         vxdb/vxdb_vx_uname_get.c \
         vxdb/vxdb_vx_uname_remove.c \
         vxdb/vxdb_vx_uname_set.c \
         vxdb/vxdb_xid_get.c

noinst_LIBRARIES = libmethods.a

libmethods_a_SOURCES = $(M_HELPER) \
                       $(M_VCD) \
                       $(M_VG) \
                       $(M_VX) \
                       $(M_VXDB)
