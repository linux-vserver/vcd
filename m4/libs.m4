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

AC_DEFUN([AC_VU_LIBVSERVER],
[
	AC_LANG_PUSH(C)
	AC_CHECK_LIB([vserver], [vx_migrate])
	AC_LANG_POP
	
	if test $ac_cv_lib_vserver_vx_migrate = no; then
		AC_MSG_ERROR([libvserver is missing! please install libvserver and try again])
	fi
])

AC_DEFUN([AC_VU_LIBUTIL],
[
	AC_LANG_PUSH(C)
	AC_CHECK_LIB([util], [forkpty])
	AC_LANG_POP
	
	if test $ac_cv_lib_util_forkpty = no; then
		AC_MSG_ERROR([libutil is missing! please reinstall glibc])
	fi
])

