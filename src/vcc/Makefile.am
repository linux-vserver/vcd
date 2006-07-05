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

LIBS = $(top_builddir)/lib/lucid/libucid.a \
       $(UTIL_LIBS) \
       $(VSERVER_LIBS) \
       $(XMLRPC_C_CLIENT_LIBS)

noinst_HEADERS = cmd.h \
                 msg.h

sbin_PROGRAMS = vcc

vcc_SOURCES = cmd.c \
              create.c \
              exec.c \
              login.c \
              main.c \
              msg.c \
              remove.c \
              rename.c \
              restart.c \
              start.c \
              status.c \
              stop.c \
              vlogin.c
