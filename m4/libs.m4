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

AC_DEFUN([AC_VU_DIETLIBC],
[
	AC_MSG_CHECKING([whether to enable dietlibc])
	
	AC_ARG_VAR(DIET,      [The 'diet' wrapper (default: diet)])
	AC_ARG_VAR(DIETFLAGS, [Flags passed to the 'diet' wrapper (default: -O)])
	
	: ${DIET:=diet}
	: ${DIETFLAGS=-Os}
	
	AC_ARG_ENABLE([dietlibc],
	              [AS_HELP_STRING([--disable-dietlibc],
	                              [do not use dietlibc resp. enforce its usage (with --enable-dietlibc) (default: autodetect dietlibc)])],
	              [case "$enableval" in
	                (yes) ac_vu_dietlibc=forced;;
	                (no)  ac_vu_dietlibc=forced_no;;
	                (*)   AC_MSG_ERROR(['$enableval' is not a valid value for --enable-dietlibc]);;
	              esac],
	              [which "$DIET" >/dev/null 2>/dev/null && ac_vu_dietlibc=detected || ac_vu_dietlibc=detected_no])
	
	if test "$ac_vu_dietlibc" = detected -a "$1"; then
		ac_vu_dietlibc_ver=$($DIET -v 2>&1 | head -n1)
		ac_vu_dietlibc_ver=${ac_vu_dietlibc_ver##*diet version }
		ac_vu_dietlibc_ver=${ac_vu_dietlibc_ver##*dietlibc-}
		
		ac_vu_dietlibc_ver_maj=${ac_vu_dietlibc_ver%%.*}
		ac_vu_dietlibc_ver_min=${ac_vu_dietlibc_ver##*.}
		ac_vu_dietlibc_ver_min=${ac_vu_dietlibc_ver_min%%[[!0-9]]*}
		
		ac_vu_dietlibc_cmp="$1"
		ac_vu_dietlibc_cmp_maj=${ac_vu_dietlibc_cmp%%.*}
		ac_vu_dietlibc_cmp_min=${ac_vu_dietlibc_cmp##*.}
		
		DIETLIBC_VERSION=$ac_vu_dietlibc_ver_maj.$ac_vu_dietlibc_ver_min
		
		let ac_vu_dietlibc_ver=ac_vu_dietlibc_ver_maj*1000+ac_vu_dietlibc_ver_min 2>/dev/null || ac_vu_dietlibc_ver=0
		let ac_vu_dietlibc_cmp=ac_vu_dietlibc_cmp_maj*1000+ac_vu_dietlibc_cmp_min
		
		test $ac_vu_dietlibc_ver -ge $ac_vu_dietlibc_cmp || use_dietlibc=detected_old
	else
		DIETLIBC_VERSION=
		ac_vu_dietlibc_ver=-1
	fi
	
	ac_vu_have_dietlibc=no
	
	case $ac_vu_dietlibc in
		detected)
			AC_MSG_RESULT([yes (autodetected, $DIETLIBC_VERSION)])
			ac_vu_have_dietlibc=yes
			;;
		
		forced)
			AC_MSG_RESULT([yes (forced)])
			ac_vu_have_dietlibc=yes
			;;
		
		detected_no)
			AC_MSG_RESULT([no (detected)])
			;;
		
		detected_old)
			AC_MSG_RESULT([no (too old; $1+ required, $DIETLIBC_VERSION found)])
			;;
		
		forced_no)
			AC_MSG_RESULT([no (forced)])
			;;
		
		*)
			AC_MSG_ERROR([internal error, use_dietlibc was "$use_dietlibc"])
			;;
	esac
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

