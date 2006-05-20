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

INCLUDES = -I$(top_srcdir)/src/lucid

LDADD = $(top_builddir)/src/lucid/libucid.a \
        -lvserver

noinst_HEADERS = tools.h

sbin_PROGRAMS = dx \
                ix \
                ns \
                nx \
                vdbm \
                vr \
                vshelper \
                vx

dx_SOURCES = dx.c msg.c
ix_SOURCES = ix.c msg.c lists.c
ns_SOURCES = ns.c msg.c
nx_SOURCES = nx.c msg.c lists.c
vdbm_SOURCES = vdbm.c msg.c
vr_SOURCES = vr.c msg.c
vshelper_SOURCES = vshelper.c msg.c
vx_SOURCES = vx.c msg.c lists.c vlogin.c
vx_LDADD   = $(LDADD) -lutil
