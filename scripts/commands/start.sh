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

: ${_HAVE_PATHCONFIG:?"Internal error: no pathconfig in start.sh"}

[ -z "${_HAVE_LIB_UTIL}" ] && source ${_LIB_UTIL}
[ -z "${_HAVE_LIB_VPS}" ]  && source ${_LIB_VPS}

start.exit_handler() {
	local is_mounted=0
	local is_setup=0
	local is_ready=0
	
	case ${STATETRACK} in
		setup)
			echo "An error occured while trying to setup the context for '${VNAME}'"
			is_setup=1
			;;
		
		mount)
			echo "An error occured while trying to mount filesystems for '${VNAME}'"
			is_setup=1
			is_mounted=1
			;;
		
		attr)
			echo "An error occured while setting context attributes for '${VNAME}'"
			is_setup=1
			is_mounted=1
			;;
		
		mountroot)
			echo "An error occured while trying to mount the root filesystem for '${VNAME}'"
			is_setup=1
			is_mounted=1
			;;
		
		init)
			echo "An error occured while trying to load init for '${VNAME}'"
			is_setup=1
			is_mounted=1
			;;
		
		done)
			is_ready=1
			;;
	esac
	
	exec &>/dev/null
	
	[ ${is_setup} -eq 1 ]   && vps.context release
	[ ${is_mounted} -eq 1 ] && vps.umount
	
	vps.unlock || :
}

start.interrupt_handler() {
	start.exit_handler
}

start.usage() {
	echo "vserver start <name>"
	echo
	echo "   <name>   Name of the vserver to start"
	echo
}

start.main() {
	[ -z "${VNAME:=$1}" ] && util.error "start: missing argument <name>"
	
	vps.loadconfig
	
	vps.running && util.error "start: vserver '${VNAME}' already running"
	
	vps.lock
	
	STATETRACK=setup
	vps.context setup
	vps.namespace new
	
	STATETRACK=mount
	vps.mount
	
	STATETRACK=attr
	vps.limit set
	vps.sched set
	vps.uname set
	vps.flags set
	
	STATETRACK=mountroot
	vps.mount root
	
	STATETRACK=init
	vps.init
	
	STATETRACK=done
	vps.unlock
}
