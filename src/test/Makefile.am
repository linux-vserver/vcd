## Copyright 2005 The vserver-utils Developers
## See AUTHORS for details
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by  *
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

INCLUDES = -I$(top_srcdir) \
           -I$(top_srcdir)/src/libinternal \
           -I$(top_srcdir)/src/libvconfig \
           -I$(top_builddir)/src

LDADD = $(top_builddir)/src/libinternal/libinternal.a \
        $(top_builddir)/src/libvconfig/libvconfig.a \
        -lvserver

noinst_HEADERS = test.h

sbin_PROGRAMS = cow

