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

AC_DEFUN([AC_LUCID],
[
	AC_MSG_CHECKING([for lucid])
	
	AC_LANG_PUSH(C)
	AC_LANG_CONFTEST([
#include <lucid/version.h>
ac_lucid_api_major=LUCID_API_MAJOR
ac_lucid_api_minor=LUCID_API_MINOR
	])
	
	eval $(${CPP} conftest.c | ${EGREP} '^ac_lucid_api_major=.*$')
	eval $(${CPP} conftest.c | ${EGREP} '^ac_lucid_api_minor=.*$')
	
	if test "x$ac_lucid_api_major" = "x" -o \
	        "x$ac_lucid_api_minor" = "x"; then
		AC_MSG_ERROR([Cannot determine lucid API version])
	else
		lucid_found=yes
		
		if test "x$1" != "x"; then
			lucid_found=no
			
			for i in $1; do
				if test "$i" = "$ac_lucid_api_major"; then
					lucid_found=yes
				fi
			done
			
		fi
		
		AC_MSG_RESULT([$ac_lucid_api_major.$ac_lucid_api_minor])
	fi
	
	AC_LANG_POP
])
