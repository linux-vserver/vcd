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

: ${_HAVE_PATHCONFIG:?"Internal error: no pathconfig in info.sh"}

[ -z "${_HAVE_LIB_UTIL}" ] && source ${_LIB_UTIL}
[ -z "${_HAVE_LIB_VPS}" ]  && source ${_LIB_VPS}


info.usage() {
	echo "Usage: vserver info <name>"
	echo
	echo "  <name>        Name of the vserver"
	echo
}

info.exit_handler() {
	:
}

info.interrupt_handler() {
	info.exit_handler
}

info.main() {
	local list
	
	[ -z "${VNAME:=$1}" ] && util.error "info: missing argument <name>"
	
	vps.loadconfig
	
	list=(VNAME VDIR VX_XID VX_INIT)
	
	for i in ${list[@]}; do
		echo -n "${i}="
		eval echo \$${i}
	done
}
