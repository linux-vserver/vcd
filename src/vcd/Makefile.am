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

SUBDIRS = methods

INCLUDES = -I$(top_srcdir)/src/lucid \
           -I$(top_srcdir)/src/libconfuse \
           -I$(top_srcdir)/src/libxmlrpc

noinst_HEADERS = log.h \
                 vxdb.h

sbin_PROGRAMS = vcd

vcd_SOURCES = main.c \
              collector.c \
              server.c \
              vxdb.c

vcd_LDADD = $(top_builddir)/src/vcd/methods/libmethods.a \
            $(top_builddir)/src/lucid/libucid.a \
            $(top_builddir)/src/libconfuse/libconfuse.a \
            $(top_builddir)/src/libxmlrpc/libxmlrpc.a \
            -lexpat