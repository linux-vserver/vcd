## Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
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

INCLUDES = -I$(top_srcdir)/src/lucid \
           -I$(top_srcdir)/src/libxmlrpc \
           -I$(top_srcdir)/src/libconfuse \
           -I$(top_srcdir)/src/vcd

M_VXDB = vx/vx_killer.c \
         vx/vx_restart.c \
         vx/vx_start.c \
         vx/vx_stop.c \
         vxdb/vxdb_create.c \
         vxdb/vxdb_dx_limit_get.c \
         vxdb/vxdb_dx_limit_remove.c \
         vxdb/vxdb_dx_limit_set.c \
         vxdb/vxdb_init_method_get.c \
         vxdb/vxdb_init_method_remove.c \
         vxdb/vxdb_init_method_set.c \
         vxdb/vxdb_init_mount_add.c \
         vxdb/vxdb_init_mount_get.c \
         vxdb/vxdb_init_mount_remove.c \
         vxdb/vxdb_list.c \
         vxdb/vxdb_nx_addr_add.c \
         vxdb/vxdb_nx_addr_get.c \
         vxdb/vxdb_nx_addr_remove.c \
         vxdb/vxdb_owner_add.c \
         vxdb/vxdb_owner_get.c \
         vxdb/vxdb_owner_remove.c \
         vxdb/vxdb_remove.c \
         vxdb/vxdb_user_add.c \
         vxdb/vxdb_user_get.c \
         vxdb/vxdb_user_remove.c \
         vxdb/vxdb_user_set.c \
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
         vxdb/vxdb_vx_pflags_add.c \
         vxdb/vxdb_vx_pflags_get.c \
         vxdb/vxdb_vx_pflags_remove.c \
         vxdb/vxdb_vx_sched_get.c \
         vxdb/vxdb_vx_sched_remove.c \
         vxdb/vxdb_vx_sched_set.c \
         vxdb/vxdb_vx_uname_get.c \
         vxdb/vxdb_vx_uname_remove.c \
         vxdb/vxdb_vx_uname_set.c

noinst_LIBRARIES = libmethods.a

libmethods_a_SOURCES = $(M_VXDB)
