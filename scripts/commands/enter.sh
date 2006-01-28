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

: ${_HAVE_PATHCONFIG:?"Internal error: no pathconfig in enter.sh"}

[ -z "${_HAVE_LIB_UTIL}" ] && source ${_LIB_UTIL}
[ -z "${_HAVE_LIB_VPS}" ]  && source ${_LIB_VPS}


enter.usage() {
	echo "Usage: vserver enter <name>"
	echo
	echo "  <name>        Name of the vserver"
	echo
}

enter.exit_handler() {
	:
}

enter.interrupt_handler() {
	enter.exit_handler
}

enter.main() {
	[ -z "${VNAME:=$1}" ] && util.error "enter: missing argument <name>"
	
	vps.loadconfig
	
	if ! vps.running; then
		util.error "enter: vserver '${VNAME}' not running"
	fi
	
	local shell=$(vps.config.get vps.shell)
	: ${shell:=/bin/bash}
	
	vps.exec ${shell}
	
}
