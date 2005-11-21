# Copyright 2005 The util-vserver Developers
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

: ${_HAVE_PATHCONFIG:?"Internal error: no pathconfig in util.sh"}

util.exec_scriplets() {
# $1 - base script path
	local path=$1
	
	[ -z "${path}" ] && return 0
	
	[ ! -e ${path} -a ! -e ${path}.d ] && return 0
	
	for i in ${path} ${path}.d/*; do
		[ -r ${i} ] && source ${i}
	done
}

util.error() {
# $1 - error message
# $2 - exit code (default: 1)
	local msg=$1
	
	[ -z "${msg}" ] && msg="(no error message)"
	
	echo "error: ${msg}" >&2
	exit ${2:-1}
}

util.warning() {
# $1 - warn message
	local msg=$1
	
	[ -z "${msg}" ] && msg="(no warning message)"
	
	echo "warning: ${msg}" >&2
}

util.array_to_list() {
	local list
	
	[ -z "$1" ] && return 0
	
	while true; do
		list="${list},${1}"
		[ -z "$2" ] && break
		shift
	done
	
	echo ${list/,}
}

_HAVE_LIB_UTIL=1
