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
m4_define([ARCHIVE_VERS],m4_include([version]))

AC_PREREQ(2.59)
AC_INIT([libarchive],ARCHIVE_VERS(),[kientzle@freebsd.org])
AC_CONFIG_HEADER([config.h])

AC_GNU_SOURCE

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# Automake
AM_INIT_AUTOMAKE([1.9 foreign dist-bzip2])
AM_MAINTAINER_MODE

# prevent building of libarchive with --disable-server
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

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

if test x$enable_server = xtrue; then

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([bzlib.h errno.h fcntl.h])
AC_CHECK_HEADERS([inttypes.h langinfo.h limits.h])
AC_CHECK_HEADERS([locale.h paths.h])
AC_CHECK_HEADERS([stdint.h stdlib.h string.h sys/ioctl.h])
AC_CHECK_HEADERS([sys/param.h sys/time.h unistd.h wchar.h zlib.h])

# Checks for libraries.
AC_CHECK_LIB(bz2,BZ2_bzDecompressInit)
AC_CHECK_LIB(z,inflate)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
# AC_TYPE_OFF_T defaults to "long", which limits us to 4GB files on
# most systems...
AC_CHECK_TYPE(off_t, [int64_t])
AC_TYPE_SIZE_T
AC_CHECK_TYPE(id_t, [unsigned long])
AC_CHECK_MEMBERS([struct stat.st_rdev])
# Check for high-resolution timestamps in struct stat
AC_CHECK_MEMBERS([struct stat.st_mtimespec.tv_nsec, struct stat.st_mtim.tv_nsec])

#
# If any of the common 64-bit types is defined, set "int64_t"
#
AC_CHECK_TYPE(__int64_t, [long long])
AC_CHECK_TYPE(_int64_t, [__int64_t])
AC_CHECK_TYPE(int64_t, [_int64_t])
# Likewise, consider defining "intmax_t" in terms of int64_t
AC_CHECK_TYPE(intmax_t, [int64_t])

#
# If any of the common 64-bit unsigned types is defined, set "uint64_t"
#
AC_CHECK_TYPE(__uint64_t, [unsigned long long])
AC_CHECK_TYPE(_uint64_t, [__uint64_t])
AC_CHECK_TYPE(u_int64_t, [_uint64_t])
AC_CHECK_TYPE(uint64_t, [u_int64_t])
# Likewise, consider defining "uintmax_t" in terms of uint64_t
AC_CHECK_TYPE(uintmax_t, [uint64_t])


AC_CHECK_DECL([EFTYPE],
		[AC_DEFINE(HAVE_EFTYPE, 1, [A possible errno value for invalid file format errors])],
		[],
		[#include <errno.h>])
AC_CHECK_DECL([EILSEQ],
		[AC_DEFINE(HAVE_EILSEQ, 1, [A possible errno value for invalid file format errors])],
		[],
		[#include <errno.h>])
AC_HEADER_TIME

# Checks for library functions.
AC_HEADER_MAJOR
AC_FUNC_MEMCMP
AC_FUNC_LSTAT
AC_FUNC_STAT
AC_FUNC_STRERROR_R
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([fchdir fchmod fchown futimes])
AC_CHECK_FUNCS([ftruncate getopt_long])
AC_CHECK_FUNCS([lchflags lchmod lchown])
AC_CHECK_FUNCS([lutimes memmove])
AC_CHECK_FUNCS([memset mkdir mkfifo nl_langinfo reallocf setlocale])
AC_CHECK_FUNCS([strchr strdup strerror strrchr timegm])

# FreeBSD's nl_langinfo supports an option to specify whether the
# current locale uses month/day or day/month ordering.  It makes the
# output a little prettier...
AC_CHECK_DECL([D_MD_ORDER],
[AC_DEFINE(HAVE_D_MD_ORDER, 1, [Define to 1 if nl_langinfo supports D_MD_ORDER])],
[],
[#if HAVE_LANGINFO_H
#include <langinfo.h>
#endif
])

# If fnmatch() exists and supports FNM_LEADING_DIR, then bsdtar uses that,
# otherwise it uses some usable (but less featureful) built-in code for
# filename pattern matching.
AC_FUNC_FNMATCH
AC_CHECK_DECL([FNM_LEADING_DIR],
[AC_DEFINE(HAVE_FNM_LEADING_DIR, 1, [Define to 1 if fnmatch(3) supports the FNM_LEADING_DIR flag])],
[],
[#if HAVE_FNMATCH
#define _GNU_SOURCE /* Required on Linux to get GNU extensions */
#include <fnmatch.h>
#endif
])

# Check for dirent.d_namlen field explicitly
# (This is a bit more straightforward than, if not quite as portable as,
# the recipe given by the autoconf maintainers.)
AC_CHECK_MEMBER(struct dirent.d_namlen,,,
[#if HAVE_DIRENT_H
#include <dirent.h>
#endif
])

# Additional requirements
AC_SYS_LARGEFILE

# $enable_server
fi

AC_CONFIG_FILES([Makefile
                 src/Makefile])

AC_OUTPUT
