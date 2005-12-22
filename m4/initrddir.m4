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

AC_DEFUN([AC_VU_INITRDDIR],
[
	AC_MSG_CHECKING([for initrddir])
	AC_ARG_WITH([initrddir],
		    [AC_HELP_STRING([--with-initrddir <DIR>],
				    [use <DIR> as directory for SysV init-files (default: $sysconfdir/init.d)])],
	            [ac_vu_initrddir=$withval],
		    [ac_vu_initrddir='$(sysconfdir)/init.d'])

	if test -n "$1"; then
		$1=$ac_vu_initrddir
		AC_SUBST($1)
	fi

	AC_MSG_RESULT($ac_vu_initrddir)
])
