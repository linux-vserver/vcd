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

: ${_HAVE_PATHCONFIG:?"Internal error: no pathconfig in exec.sh"}

[ -z "${_HAVE_LIB_UTIL}" ] && source ${_LIB_UTIL}
[ -z "${_HAVE_LIB_VPS}" ]  && source ${_LIB_VPS}


exec.usage() {
	echo "Usage: vserver exec <name> <command> <args>*"
	echo
	echo "  <name>        Name of the vserver"
	echo "  <command>     Command to execute in vserver <name>"
	echo "  <args>        Command line arguments for <command>"
	echo
}

exec.exit_handler() {
	vps.unlock || :
}

exec.interrupt_handler() {
	exec.exit_handler
}

exec.main() {
	[ -z "${VNAME:=$1}" ] && util.error "exec: missing argument <name>"
	[ -z "$2" ] && util.error "exec: missing argument <command>"
	shift
	
	vps.loadconfig
	
	if ! vps.running; then
		util.error "exec: vserver '${VNAME}' not running"
	fi
	
	vps.lock
	
	STATETRACK=exec
	vps.exec "$@"
	
	STATETRACK=unlock
	vps.unlock
	
	STATETRACK=done
}
