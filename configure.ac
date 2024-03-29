# Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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
AC_INIT(vcd, 1.0_pre1, hollow@gentoo.org)
AC_GNU_SOURCE

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# Automake
AM_INIT_AUTOMAKE([1.9 gnu dist-bzip2])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Parts to build
AC_ARG_ENABLE([server],
              [AC_HELP_STRING([--disable-server],
                              [Disable the VServer Control Daemon])],
              [case "${enableval}" in
                yes) enable_server=true;;
                no)  enable_server=false;;
                *)   AC_MSG_ERROR(bad value ${enableval} for --enable-server);;
              esac],
              [enable_server=true])
AM_CONDITIONAL(ENABLE_SERVER, test x$enable_server = xtrue)

AC_ARG_ENABLE([client],
              [AC_HELP_STRING([--disable-client],
                              [Disable the VServer Control Client])],
              [case "${enableval}" in
                yes) enable_client=true;;
                no)  enable_client=false;;
                *)   AC_MSG_ERROR(bad value ${enableval} for --enable-client);;
              esac],
              [enable_client=true])
AM_CONDITIONAL(ENABLE_CLIENT, test x$enable_client = xtrue)

# Checks for libraries.
# syscalls not in glibc < 2.4
AC_CHECK_FUNCS([fchmodat fchownat linkat mkdirat openat symlinkat])

# confuse
AC_PATH_PROG(CONFUSE_CONFIG, confuse-config)
if test -z "$CONFUSE_CONFIG"; then
	AC_MSG_ERROR([confuse-config not found])
fi

CONFUSE_LIBS="$($CONFUSE_CONFIG --libs)"

AC_SUBST(CONFUSE_LIBS)

# lucid
AC_CHECK_LIB(ucid, str_path_concat,
	LUCID_LIBS="-lucid", AC_MSG_ERROR([lucid not found]),)

AC_SUBST(LUCID_LIBS)

AC_DEFINE(_LUCID_PRINTF_MACROS)
AC_DEFINE(_LUCID_SCANF_MACROS)

# vserver
AC_CHECK_LIB(vserver, vx_stat,
	VSERVER_LIBS="-lvserver", AC_MSG_ERROR([libvserver >= 2.0 not found]),)

AC_SUBST(VSERVER_LIBS)

# xmlrpc-c
AC_PATH_PROG(XMLRPC_C_CONFIG, xmlrpc-c-config)
if test -z "$XMLRPC_C_CONFIG"; then
	AC_MSG_ERROR([xmlrpc-c-config not found])
fi

XMLRPC_C_VERSION="$($XMLRPC_C_CONFIG --version)"
XMLRPC_C_MAJOR="${XMLRPC_C_VERSION/.*}"
XMLRPC_C_MINOR="${XMLRPC_C_VERSION#*.}"
XMLRPC_C_MINOR="${XMLRPC_C_MINOR/.*}"

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

if test -z "$XMLRPC_C_CLIENT_LIBS"; then
	AC_MSG_ERROR([no xmlrpc-c client library found])
fi

AC_SUBST(XMLRPC_C_CLIENT_LIBS)
AC_SUBST(XMLRPC_C_SERVER_LIBS)

# util
if test x$enable_client = xtrue; then
AC_CHECK_FUNC(openpty, UTIL_LIBS="",[
	AC_CHECK_LIB(util, openpty, UTIL_LIBS="-lutil", AC_MSG_ERROR([libutil not found]))
])

AC_SUBST(UTIL_LIBS)
fi

# sqlite3
if test x$enable_server = xtrue; then
AC_CHECK_LIB(sqlite3, sqlite3_open,
	SQLITE3_LIBS="-lsqlite3", AC_MSG_ERROR([sqlite3 not found]))

AC_SUBST(SQLITE3_LIBS)
fi

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

# Compiler settings
CPPFLAGS="${CPPFLAGS} -D_GNU_SOURCE"
CFLAGS="${CFLAGS} -std=gnu99 -pedantic"
CFLAGS="${CFLAGS} -Wall -Wpointer-arith -Wcast-qual -Winline"
CFLAGS="${CFLAGS} -Wredundant-decls -Wcast-align -Wno-unused-parameter"

# Site settings
AC_MSG_CHECKING([for vserver base directory])
AC_ARG_WITH([vbasedir],
            [AC_HELP_STRING([--with-vbasedir=DIR],
                            [vserver base directory (default: /vservers)])],
            [vbasedir=$withval],
            [vbasedir=/vservers])
AC_MSG_RESULT([$vbasedir])

AC_SUBST(vbasedir)

PATH_CPPFLAGS="$PATH_CPPFLAGS -DSYSCONFDIR='\"\$(sysconfdir)\"'"
PATH_CPPFLAGS="$PATH_CPPFLAGS -DPKGDATADIR='\"\$(pkgdatadir)\"'"
PATH_CPPFLAGS="$PATH_CPPFLAGS -DVBASEDIR='\"\$(vbasedir)\"'"
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
                 vbasedir: $vbasedir
            localstatedir: $localstatedir
"])

# Output
AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 src/vcc/vcc/Makefile
                 src/vcc/vcdadm/Makefile
                 src/vcc/vce/Makefile
                 src/vcc/vgc/Makefile
                 src/vcc/Makefile
                 src/vcd/methods/Makefile
                 src/vcd/Makefile
                 src/vshelper/Makefile
                 src/vxpasswd/Makefile
                 src/Makefile
                 syscall/Makefile
                 test/Makefile])

AC_OUTPUT

AC_MSG_NOTICE([$SUMMARY])
