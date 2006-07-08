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

if ENABLE_SERVER

SUBDIRS = methods

INCLUDES = -I$(top_srcdir)/lib/lucid

EXTRA_DIST = vcd.conf \
             vshelper.conf

AM_CPPFLAGS = $(PATH_CPPFLAGS)

noinst_HEADERS = auth.h \
                 cfg.h \
                 lists.h \
                 log.h \
                 methods.h \
                 validate.h \
                 vxdb.h

sbin_PROGRAMS = vcd \
                vshelper

vcd_SOURCES = auth.c \
              cfg.c \
              lists.c \
              log.c \
              main.c \
              methods.c \
              validate.c \
              vxdb.c

vcd_LDADD = $(top_builddir)/src/vcd/methods/libmethods.a \
            $(top_builddir)/lib/libarchive/src/libarchive.a \
            $(top_builddir)/lib/lucid/libucid.a \
            $(BZ2_LIBS) $(Z_LIBS) \
            $(CONFUSE_LIBS) \
            $(DBI_LIBS) \
            $(VSERVER_LIBS) \
            $(XMLRPC_C_SERVER_LIBS)

vshelper_SOURCES = vshelper.c

vshelper_LDADD = $(CONFUSE_LIBS) \
                 $(XMLRPC_C_CLIENT_LIBS)

pkglib_PROGRAMS = gensql

gensql_SOURCES = gensql.c
gensql_LDADD = $(top_builddir)/lib/lucid/libucid.a

install-data-local:
	$(install_sh)    -m 600 $(srcdir)/vcd.conf $(DESTDIR)$(sysconfdir)/vcd.conf
	$(install_sh)    -m 600 $(srcdir)/vshelper.conf $(DESTDIR)$(sysconfdir)/vshelper.conf
	$(mkinstalldirs) -m 755 $(DESTDIR)$(localstatedir)/vcd
	$(mkinstalldirs) -m 000 $(DESTDIR)$(vserverdir)

endif
