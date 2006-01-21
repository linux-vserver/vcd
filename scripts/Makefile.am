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

pkgdata_DATA = pathconfig

nobase_pkgdata_DATA = commands/enter.sh \
                      commands/exec.sh \
                      commands/info.sh \
                      commands/login.sh \
                      commands/procfs.sh \
                      commands/release.sh \
                      commands/start.sh \
                      commands/status.sh \
                      commands/stop.sh \
                      defaults/fstab \
                      defaults/mtab \
                      defaults/procfs.conf \
                      lib/fs.sh \
                      lib/util.sh \
                      lib/vps.sh \
                      lib/vs.sh

sbin_SCRIPTS = vpstree \
               vserver \
               vtop

include $(top_srcdir)/m4/pathconfig.am

BUILT_SOURCES = pathconfig
CLEANFILES    = pathconfig

EXTRA_DIST = $(nobase_pkgdata_DATA) \
             $(sbin_SCRIPTS) \
             pathconfig.in


pathconfig: pathconfig.in
	$(pathconfig_SED) $(srcdir)/pathconfig.in > pathconfig
