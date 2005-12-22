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

: ${_HAVE_PATHCONFIG:?"Internal error: no pathconfig in stop.sh"}

[ -z "${_HAVE_LIB_UTIL}" ] && source ${_LIB_UTIL}
[ -z "${_HAVE_LIB_VPS}" ]  && source ${_LIB_VPS}

stop.exit_handler() {
	local is_ready=0
	local rc=0
	
	case ${STATETRACK} in
		halt)
			echo "An error occured while trying to halt vserver '${VNAME}'"
			;;
		
		umount)
			if vps.running; then
				echo "An error occured while trying to unmount filesystems for '${VNAME}'"
			fi
			;;
		
		done)
			is_ready=1
			;;
	esac
	
	# silently ignore errors below
	exec &> /dev/null
	
	vps.unlock || :
}

stop.interrupt_handler() {
	stop.exit_handler
}

stop.usage() {
	echo "vserver stop <name>"
	echo
	echo "   <name>   Name of the vserver to stop"
	echo
}

stop.main() {
	local cnt=0
	
	[ -z "${VNAME:=$1}" ] && util.error "stop: missing argument <name>"
	
	vps.loadconfig
	
	vps.running || util.error "stop: vserver '${VNAME}' not running"
	
	vps.lock
	
	STATETRACK=halt
	vps.halt
	
	while [ ${cnt} -lt ${VX_TIMEOUT_KILL} ]; do
		vps.running || break
		
		echo -n "."
		
		sleep 2
		let cnt+=2
	done
	
	echo
	
	if vps.running; then
		STATETRACK=kill
		util.warning "vserver '${VNAME}' still running after timeout"
		util.warning "trying to kill..."
		
		vps.kill
		
		sleep 5
		
		if vps.running; then
			util.warning "vserver '${VNAME}' still running after kill"
			util.warning "please investigate manually"
		fi
		
		exit 1
	fi
	
	# silently ignore errors below
	exec &>/dev/null
	
	STATETRACK=umount
	vps.umount
	
	STATETRACK=done
	vps.unlock
}
