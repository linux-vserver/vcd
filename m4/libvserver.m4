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

AC_DEFUN([AC_LIBVSERVER],
[
	AC_MSG_CHECKING([for libvserver])
	
	AC_LANG_PUSH(C)
	AC_LANG_CONFTEST([
#include <vserver.h>
ac_libvserver_api_major=LIBVSERVER_API_MAJOR
ac_libvserver_api_minor=LIBVSERVER_API_MINOR
	])
	
	eval $(${CPP} conftest.c | ${EGREP} '^ac_libvserver_api_major=.*$')
	eval $(${CPP} conftest.c | ${EGREP} '^ac_libvserver_api_minor=.*$')
	
	if test "x$ac_libvserver_api_major" = "x" -o \
	        "x$ac_libvserver_api_minor" = "x"; then
		AC_MSG_ERROR([Cannot determine libvserver API version])
	else
		libvserver_found=yes
		
		if test "x$1" != "x"; then
			libvserver_found=no
			
			for i in $1; do
				if test "$i" = "$ac_libvserver_api_major"; then
					libvserver_found=yes
				fi
			done
			
		fi
		
		if test "x$libvserver_found" = "yes" ; then 
			AC_MSG_RESULT([$ac_libvserver_api_major.$ac_libvserver_api_minor])
		else
			AC_MSG_RESULT([no])
		fi
	fi
	
	AC_LANG_POP
])
