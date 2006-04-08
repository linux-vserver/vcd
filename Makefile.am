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

include $(top_srcdir)/m4/pathconfig.am

SUBDIRS = etc \
          src

BUILT_SOURCES = pathconfig.h
CLEANFILES    = pathconfig.h

EXTRA_DIST = pathconfig.h.in

pathconfig.h: pathconfig.h.in
	@SED@ $(PATHCONFIG_SCRIPT) $(srcdir)/pathconfig.h.in > ./pathconfig.h

install-data-hook: install-vdirbase install-misc

install-vdirbase:
	$(mkinstalldirs) -m 000 $(DESTDIR)$(vdirbase)

install-misc:
	$(mkinstalldirs) -m 000 $(DESTDIR)/var/lib/vserver
	$(mkinstalldirs) -m 000 $(DESTDIR)/var/lock/vservers
