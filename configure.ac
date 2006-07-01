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

# Generic configuration
AC_PREREQ(2.59)
AC_INIT(vserver-utils, 2.0.0, hollow@gentoo.org)
AC_CONFIG_HEADER([config.h])
AC_GNU_SOURCE

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_SUBDIRS(lib/libarchive)

# Automake
AM_INIT_AUTOMAKE([1.9 gnu dist-bzip2])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_FUNC(dlopen, DL_LIBS="",[
	AC_CHECK_LIB(dl, dlopen, DL_LIBS="-ldl", AC_MSG_ERROR([libdl not found]))
])

AC_SUBST(DL_LIBS)

AC_CHECK_FUNC(openpty, UTIL_LIBS="",[
	AC_CHECK_LIB(util, openpty, UTIL_LIBS="-lutil", AC_MSG_ERROR([libutil not found]))
])

AC_SUBST(UTIL_LIBS)

AC_PATH_PROG(CONFUSE_CONFIG, confuse-config)
if test -z "$CONFUSE_CONFIG"; then
	AC_MSG_ERROR([confuse-config not found])
fi

CONFUSE_LIBS="$($CONFUSE_CONFIG --libs)"

AC_SUBST(CONFUSE_LIBS)

AC_CHECK_LIB(dbi, dbi_result_get_int,
	DBI_LIBS="$DL_LIBS -ldbi", AC_MSG_ERROR([libdbi >= 0.8 not found]),)

AC_MSG_CHECKING([for libdbi sqlite3 driver])
LIBS="$DBI_LIBS"
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(,[[dbi_initialize(0);dbi_conn_new("sqlite3");]])],
	[AC_MSG_RESULT([yes])],
	[AC_MSG_ERROR([sqlite3 driver not found])])

AC_SUBST(DBI_LIBS)

AC_CHECK_LIB(vserver, vx_clone_namespace, 
	VSERVER_LIBS="-lvserver", AC_MSG_ERROR([libvserver >= 2.0 not found]),)

AC_SUBST(VSERVER_LIBS)

AC_PATH_PROG(XMLRPC_C_CONFIG, xmlrpc-c-config)
if test -z "$XMLRPC_C_CONFIG"; then
	AC_MSG_ERROR([xmlrpc-c-config not found])
fi

XMLRPC_C_VERSION="$($XMLRPC_C_CONFIG --version)"
XMLRPC_C_MAJOR="${XMLRPC_C_VERSION/.*}"
XMLRPC_C_MINOR="${XMLRPC_C_VERSION#*.}"

AC_MSG_CHECKING([for xmlrpc-c >= 1.05])

if test $XMLRPC_C_MAJOR -lt 1; then
	AC_MSG_ERROR([xmlrpc-c-$XMLRPC_VERSION does not match])
fi

if test $XMLRPC_C_MAJOR -eq 1 && test $XMLRPC_C_MINOR -lt 05; then
	AC_MSG_ERROR([xmlrpc-c-$XMLRPC_VERSION does not match])
fi

AC_MSG_RESULT([found])

XMLRPC_C_CLIENT_LIBS="$($XMLRPC_C_CONFIG client --libs)"
XMLRPC_C_SERVER_LIBS="$($XMLRPC_C_CONFIG abyss-server --libs)"

AC_SUBST(XMLRPC_C_CLIENT_LIBS)
AC_SUBST(XMLRPC_C_SERVER_LIBS)

AC_CHECK_LIB(bz2, BZ2_bzDecompressInit,
	BZ2_LIBS="-lbz2", AC_MSG_ERROR([libbz2 not found]),)

AC_CHECK_LIB(z, inflate,
	BZ2_LIBS="-lz", AC_MSG_ERROR([libz not found]),)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

# Compiler settings
CPPFLAGS="${CPPFLAGS} -D_GNU_SOURCE"
CFLAGS="${CFLAGS} -std=gnu99 -pedantic"
CFLAGS="${CFLAGS} -Wall -Wextra -Wpointer-arith -Wcast-qual -Winline"
CFLAGS="${CFLAGS} -Wredundant-decls -Wcast-align -Wno-unused-parameter"

# Site settings
AC_MSG_CHECKING([for vserver base directory])
AC_ARG_WITH([vserverdir],
            [AC_HELP_STRING([--with-vserverdir=DIR],
                            [vserver base directory (default: /vservers)])],
            [vserverdir=$withval],
            [vserverdir=/vservers])
AC_MSG_RESULT([$vserverdir])

AC_SUBST(vserverdir)

PATH_CPPFLAGS="$PATH_CPPFLAGS -DSYSCONFDIR='\"\$(sysconfdir)\"'"
PATH_CPPFLAGS="$PATH_CPPFLAGS -DPKGDATADIR='\"\$(pkgdatadir)\"'"
PATH_CPPFLAGS="$PATH_CPPFLAGS -DVSERVERDIR='\"\$(vserverdir)\"'"
PATH_CPPFLAGS="$PATH_CPPFLAGS -DLOCALSTATEDIR='\"\$(localstatedir)\"'"

AC_SUBST(PATH_CPPFLAGS)

# Final info page
AC_CONFIG_COMMANDS_PRE([SUMMARY="$PACKAGE_STRING configured successfully:

                       CC: $CC ($($CC --version | head -n1))
                 CPPFLAGS: '$CPPFLAGS'
                   CFLAGS: '$CFLAGS'
                    build: $build
                     host: $host
                   target: $target

                   prefix: $prefix
               sysconfdir: $sysconfdir
               vserverdir: $vserverdir
            localstatedir: $localstatedir
"])

# Output
AC_CONFIG_FILES([Makefile
                 scripts/Makefile
                 lib/lucid/Makefile
                 lib/Makefile
                 src/tools/Makefile
                 src/vcd/methods/Makefile
                 src/vcd/Makefile
                 src/vstatd/Makefile
                 src/Makefile])

AC_OUTPUT

AC_MSG_NOTICE([$SUMMARY])
