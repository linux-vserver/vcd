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
           -I$(top_srcdir)/src/vcd

M_VXDB = vxdb/get.c \
         vxdb/set.c

METHODS = $(M_VXDB)

noinst_LIBRARIES = libmethods.a

noinst_HEADERS = auth.h \
                 methods.h

libmethods_a_SOURCES = auth.c \
                       methods.c \
                       $(METHODS)
