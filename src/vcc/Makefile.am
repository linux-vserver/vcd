## Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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

if ENABLE_CLIENT

SUBDIRS = vcc \
          vcdadm \
          vce \
          vgc

EXTRA_DIST = vcc.conf

noinst_HEADERS = vcc.h

sbin_PROGRAMS = vcc/vcc \
                vcdadm/vcdadm \
                vce/vce \
                vgc/vgc

COMMON_LIBS = $(LUCID_LIBS) \
              $(CONFUSE_LIBS) \
              $(UTIL_LIBS) \
              $(VSERVER_LIBS) \
              $(XMLRPC_C_CLIENT_LIBS)

vcc_vcc_SOURCES  = main.c vlogin.c
vcc_vcc_LDADD    = vcc/libcmd.a $(COMMON_LIBS)
vcc_vcc_CPPFLAGS = -I$(srcdir)/vcc $(PATH_CPPFLAGS)

vcdadm_vcdadm_SOURCES  = main.c
vcdadm_vcdadm_LDADD    = vcdadm/libcmd.a $(COMMON_LIBS)
vcdadm_vcdadm_CPPFLAGS = -I$(srcdir)/vcdadm $(PATH_CPPFLAGS)

vce_vce_SOURCES  = main.c
vce_vce_LDADD    = vce/libcmd.a $(COMMON_LIBS)
vce_vce_CPPFLAGS = -I$(srcdir)/vce $(PATH_CPPFLAGS)

vgc_vgc_SOURCES  = main.c
vgc_vgc_LDADD    = vgc/libcmd.a $(COMMON_LIBS)
vgc_vgc_CPPFLAGS = -I$(srcdir)/vgc $(PATH_CPPFLAGS)

install-data-local:
	$(install_sh) -m 600 $(srcdir)/vcc.conf $(DESTDIR)$(sysconfdir)/vcc.conf

endif
