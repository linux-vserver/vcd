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

: ${_HAVE_PATHCONFIG:?"Internal error: no pathconfig in fs.sh"}

[ -z "${_HAVE_LIB_UTIL}" ] && source ${_LIB_UTIL}


fs.best_file() {
# $* - files to be found by priority
	local found
	
	while [ $# -ne 0 ]; do
		[ -e $1 ] && found=$1 && break
		shift
	done
	
	echo ${found}
}

fs.lock() {
	local lockfile=$1
	
	[ -z "${lockfile}" ] && util.error "fs.lock: argument <lockfile> missing"
	
	# Create random ID string for lockfile
	LOCKKEY=${RANDOM}
	
	if [ -e "${lockfile}" ]; then
		util.error "fs.lock: lockfile '${lockfile}' exists"
	else
		echo ${LOCKKEY} > ${lockfile}
	fi
}

fs.locked() {
# return codes:
# 0 - unlocked
# 1 - locked,mine
# 2 - locked,foreign
	local lockfile=$1
	
	[ -z "${lockfile}" ] && util.error "fs.locked: argument <lockfile> missing"
	[ -z "${LOCKKEY}" ]  && util.error "fs.locked: LOCKKEY missing"
	
	if [ -e "${lockfile}" ]; then
		if [ "$(<${lockfile})" == "${LOCKKEY}" ]; then
			return 1
		else
			return 2
		fi
	else
		return 0
	fi
}

fs.unlock() {
# return codes:
# 0 - noop
# 1 - unlocked,mine
# 2 - locked,foreign
	local lockfile=$1
	
	[ -z "${lockfile}" ] && util.error "fs.unlock: argument <lockfile> missing"
	[ -z "${LOCKKEY}" ]  && util.error "fs.locked: LOCKKEY missing"
	
	if [ -e "${lockfile}" ]; then
		if [ "$(<${lockfile})" == "${LOCKKEY}" ]; then
			rm -f ${lockfile}
			return 1
		else
			return 2
		fi
	else
		return 0
	fi
}

_HAVE_LIB_FS=1
