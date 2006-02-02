# Copyright 2005 The vserver-utils Developers
# See AUTHORS for details
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by  *
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

AC_DEFUN([AC_VU_VCONFIG_BACKEND],
[
	AC_MSG_CHECKING([which backend to use for libvconfig])
	
	AC_ARG_WITH([vconfig],
	            [AS_HELP_STRING([--with-vconfig],
	                            [Backend for libvconfig (valid values: plain; default: plain)])],
	            [case "$enableval" in
	              (single) AC_DEFINE([LIBVCONFIG_BACKEND_SINGLE], [], [libvconfig backend]);;
	              (no)     AC_MSG_ERROR([You cannot disable libvconfig]);;
	              (*)      AC_MSG_ERROR(['$enableval' is not a valid value for --with-vconfig]);;
	            esac],
	            [enableval=single; AC_DEFINE([LIBVCONFIG_BACKEND_SINGLE])])
	
	ac_vu_vconfig_backend=$enableval
	AC_MSG_RESULT([$enableval])
])
