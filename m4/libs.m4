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

AC_DEFUN([AC_VU_DIETLIBC],
[
	AC_MSG_CHECKING([for dietlibc])
	
	AC_ARG_VAR(DIET,      [The 'diet' wrapper (default: diet)])
	AC_ARG_VAR(DIETFLAGS, [Flags passed to the 'diet' wrapper (default: -O)])
	
	: ${DIET:=diet}
	: ${DIETFLAGS=-Os}
	
	CC="${DIET} ${DIETFLAGS} ${CC}"
	
	AC_LANG_PUSH(C)
	AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#ifndef __dietlibc__
#error
#endif
	]])],
	[ac_vu_have_dietlibc=yes],
	[ac_vu_have_dietlibc=no])
	
	if test "$ac_vu_have_dietlibc" = "no"; then
		AC_MSG_ERROR([dietlibc is missing! please install dietlibc and try again])
	else
		AC_MSG_RESULT([ok])
	fi
	
	AC_LANG_POP
])

AC_DEFUN([AC_VU_LIBVCONFIG],
[
	AC_MSG_CHECKING([which backend to use for libvconfig])
	
	AC_ARG_WITH([vconfig],
	            [AS_HELP_STRING([--with-vconfig],
	                            [Backend for libvconfig (valid values: plain; default: plain)])],
	            [case "$enableval" in
	              (single) AC_DEFINE([LIBVCONFIG_BACKEND_SINGLE], [], [libvconfig backend]);;
	              (no)     AC_MSG_ERROR([You cannot disable libvconfig]);;
	              (*)      AC_MSG_ERROR(['$enableval' is not a valid value for --with-libvconfig]);;
	            esac],
	            [enableval=single; AC_DEFINE([LIBVCONFIG_BACKEND_SINGLE])])
	
	ac_vu_libvconfig_backend=$enableval
	AC_MSG_RESULT([$enableval])
])

AC_DEFUN([AC_VU_LIBVSERVER],
[
	AC_LANG_PUSH(C)
	AC_CHECK_LIB([vserver],
	             [vx_migrate],
	             [ac_vu_have_libvserver=yes],
	             [ac_vu_have_libvserver=no])
	AC_LANG_POP
	
	if test $ac_vu_have_libvserver = no; then
		AC_MSG_ERROR([libvserver is missing! please install libvserver and try again])
	fi
])

AC_DEFUN([AC_VU_LIBUTIL],
[
	AC_LANG_PUSH(C)
	AC_CHECK_LIB([util],
	             [forkpty],
	             [ac_vu_have_libutil=yes],
	             [ac_vu_have_libutil=no])
	AC_LANG_POP
	
	if test $ac_vu_have_libutil = no; then
		AC_MSG_ERROR([libutil is missing! please reinstall glibc])
	fi
])
