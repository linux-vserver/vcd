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

# Generic configuration
AC_PREREQ(2.59)
AC_INIT(vserver-utils, 1.0.2, hollow@gentoo.org)
AC_CONFIG_HEADER([config.h])
AC_GNU_SOURCE

# Automake
AM_INIT_AUTOMAKE([1.9 gnu dist-bzip2])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S

AC_VU_PATHPROG(CP,  cp)
AC_VU_PATHPROG(SED, sed)

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# Compiler settings
CFLAGS="${CFLAGS} -W -Wall -std=gnu99 -pedantic"

# Site settings
AC_VU_INITRDDIR(initrddir)
AC_VU_VDIRBASE(vdirbase)

# Output
AC_CONFIG_FILES([Makefile
                 etc/Makefile
                 scripts/Makefile
                 src/Makefile])

AC_OUTPUT
