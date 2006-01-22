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

: ${_HAVE_PATHCONFIG:?"Internal error: no pathconfig in reload.sh"}

[ -z "${_HAVE_LIB_UTIL}" ] && source ${_LIB_UTIL}
[ -z "${_HAVE_LIB_VPS}" ]  && source ${_LIB_VPS}

reload.exit_handler() {
	:
}

reload.interrupt_handler() {
	reload.exit_handler
}

reload.usage() {
	echo "vserver reload <name>"
	echo
	echo "   <name>   Name of the vserver to reload"
	echo
}

reload.main() {
	[ -z "${VNAME:=$1}" ] && util.error "reload: missing argument <name>"
	
	vps.loadconfig
	
	vps.running && util.error "reload: vserver '${VNAME}' already running"
	
	vps.lock
	
	vps.limit set
	vps.sched set
	vps.uname set
	vps.flags set
	
	vps.unlock
}
