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

INCLUDES = -I$(top_srcdir)/lib/lucid

LIBS = $(top_builddir)/src/vcd/methods/libmethods.a \
       $(top_builddir)/lib/lucid/libucid.a \
       $(CONFUSE_LIBS) \
       $(DBI_LIBS) \
       $(VSERVER_LIBS) \
       $(XMLRPC_C_SERVER_LIBS)

sysconf_DATA = vcd.conf
pkgdata_DATA = vxdb.sql

EXTRA_DIST = $(sysconf_DATA) \
             $(pkgdata_DATA)

AM_CPPFLAGS = $(PATH_CPPFLAGS)

noinst_HEADERS = auth.h \
                 cfg.h \
                 lists.h \
                 log.h \
                 methods.h \
                 validate.h \
                 vxdb.h

sbin_PROGRAMS = vcd

vcd_SOURCES = auth.c \
              cfg.c \
              lists.c \
              log.c \
              main.c \
              methods.c \
              validate.c \
              vxdb.c

install-data-hook:
	$(mkinstalldirs) -m 755 $(DESTDIR)$(localstatedir)/vcd
	$(mkinstalldirs) -m 000 $(DESTDIR)$(vserverdir)
