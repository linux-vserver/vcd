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

INCLUDES = -I$(top_srcdir)/lib/lucid

sysconf_DATA = vce.conf

EXTRA_DIST = $(sysconf_DATA)

AM_CPPFLAGS = $(PATH_CPPFLAGS)

noinst_HEADERS = cmd.h \
                 msg.h

sbin_PROGRAMS = vce

vce_SOURCES = cmd.c \
              dx_limit.c \
              init_method.c \
              main.c \
              mount.c \
              msg.c \
              nx_addr.c \
              nx_broadcast.c \
              vx_bcaps.c \
              vx_ccaps.c \
              vx_flags.c \
              vx_limit.c \
              vx_sched.c \
              vx_uname.c

vce_LDADD = $(top_builddir)/lib/lucid/libucid.a \
            $(CONFUSE_LIBS) \
            $(UTIL_LIBS) \
            $(VSERVER_LIBS) \
            $(XMLRPC_C_CLIENT_LIBS)
