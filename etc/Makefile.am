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

initrd_SCRIPTS = vservers

nobase_pkgdata_DATA = examples/context.conf

install-data-hook: install-pkgconfdir install-vdirbase-link

install-pkgconfdir:
	$(mkinstalldirs) -m 755 $(DESTDIR)$(pkgconfdir)
	$(mkinstalldirs) -m 755 $(DESTDIR)$(pkgconfdir)/.defaults

install-vdirbase-link:
	f=$(DESTDIR)$(pkgconfdir)/.defaults/vdirbase; \
	test -e "$$f" || ln -s $(vdirbase) "$$f"
