# Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

# These are defined by autoconf:
# - bindir
# - libdir
# - localstatedir
# - mandir
# - sbindir
# - sysconfdir
#
# We also set those:
# - lockdir
# - vdirbase

AC_DEFUN([AC_VU_PATHCONFIG],
[
	lockdir='${localstatedir}/lock'
	
	AC_MSG_CHECKING([for vserver base directory])
	AC_ARG_WITH([vdirbase],
	            [AC_HELP_STRING([--with-vdirbase=DIR],
	                            [default vserver base directory (default: /vservers)])],
	            [ac_vu_vdirbase=$withval],
	            [ac_vu_vdirbase=/vservers])
	
	AC_MSG_RESULT([$ac_vu_vdirbase])

	vdirbase=$ac_vu_vdirbase
	
	AC_SUBST(lockdir)
	AC_SUBST(vdirbase)
])
