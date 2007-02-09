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

EXTRA_DIST = vcd.conf

INCLUDES = -I$(top_srcdir)/syscall

AM_CPPFLAGS = $(PATH_CPPFLAGS)

noinst_HEADERS = auth.h \
                 cfg.h \
                 lists.h \
                 methods.h \
                 syscall-compat.h \
                 validate.h \
                 vxdb.h \
                 vxdb-tables.h

sbin_PROGRAMS = vcd

vcd_SOURCES = auth.c \
              cfg.c \
              lists.c \
              main.c \
              methods.c \
              syscall-compat.c \
              validate.c \
              vxdb.c

vcd_LDADD = $(top_builddir)/src/vcd/methods/libmethods.a \
            $(CONFUSE_LIBS) \
            $(LUCID_LIBS) \
            $(SQLITE3_LIBS) \
            $(VSERVER_LIBS) \
            $(XMLRPC_C_SERVER_LIBS)

install-data-local:
	$(install_sh)    -m 600 $(srcdir)/vcd.conf $(DESTDIR)$(sysconfdir)/vcd.conf
	$(mkinstalldirs) -m 755 $(DESTDIR)$(localstatedir)/vcd
	$(mkinstalldirs) -m 000 $(DESTDIR)$(vbasedir)

endif
