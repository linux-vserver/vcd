# Copyright 2005 The vserver-utils Developers
# See AUTHORS for details
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

: ${_HAVE_PATHCONFIG:?"Internal error: no pathconfig in procfs.sh"}

[ -z "${_HAVE_LIB_FS}" ]   && source ${_LIB_FS}
[ -z "${_HAVE_LIB_UTIL}" ] && source ${_LIB_UTIL}

procfs.exit_handler() {
	:
}

procfs.interrupt_handler() {
	procfs.exit_handler
}

procfs.usage() {
	echo "vserver procfs hide|unhide [<conf>]"
	echo
	echo "   <conf>   procfs configuration file"
	echo
	echo "Return values:"
	echo "    0  No errors"
	echo "    1  Configuration error"
	echo "    2  Some files were changed but errors occured on other ones"
	echo "    3  Operation failed on every file"
}

procfs.main() {
	local subcmd=$1 && shift
	local flags opts
	local ok=1 passed=0
	
	[ -z "${subcmd}" ] && util.error "procfs.start: missing argument <subcmd>"
	
	if [ "${subcmd}" == "hide" ]; then
		${_VATTR} -S -cr -f HIDE /proc || exit 2
		exit 0
	fi
	
	[ "${subcmd}" != "unhide" ] && util.error "unknown subcommand '${subcmd}'"
	
	while read line; do
		case "${line}" in
			(\#*) continue;;
			(\~*) flags=(ADMIN WATCH HIDE); line=${line#\~};;
			(-*)  flags=(ADMIN       HIDE); line=${line#-};;
			(:*)  flags=(      WATCH HIDE); line=${line#:};;
			(!*)  flags=(            HIDE); line=${line#!};;
			(+*)  flags=(           ~HIDE); line=${line#+};;
			(*)   flags=(           ~HIDE);;
		esac
		
		case "${line}" in
			(*/)  opts="-r";;
			(*)   ;;
		esac
		
		set -- ${line}
		
		[ ! -e "$1" ] && continue
		
		if ${_VATTR} -S ${opts} -f $(util.array_to_list ${flags[@]}) "$1"; then
			passed=1
		else
			ok=0
		fi
	done < ${__PKGCONFDIR}/procfs.conf
	
	[ ${ok} -eq 1 ]     && exit 0
	[ ${passed} -eq 1 ] && exit 2
	                       exit 3
}
