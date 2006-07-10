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

noinst_HEADERS = tools.h

sbin_PROGRAMS = dx \
                ix \
                ns \
                nx \
                vr \
                vx \
                vserver-stat

TOOLLIBS = $(top_builddir)/lib/lucid/libucid.a \
           $(VSERVER_LIBS)

vserver_stat_SOURCES = vserver-stat.c
vserver_stat_LDADD = $(TOOLLIBS)

dx_SOURCES = dx.c msg.c
dx_LDADD = $(TOOLLIBS)

ix_SOURCES = ix.c msg.c lists.c
ix_LDADD = $(TOOLLIBS)

ns_SOURCES = ns.c msg.c
ns_LDADD = $(TOOLLIBS)

nx_SOURCES = nx.c msg.c lists.c
nx_LDADD = $(TOOLLIBS)

vr_SOURCES = vr.c msg.c
vr_LDADD = $(TOOLLIBS)

vx_SOURCES = vx.c msg.c lists.c
vx_LDADD = $(TOOLLIBS)
