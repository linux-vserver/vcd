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

: ${_HAVE_PATHCONFIG:?"Internal error: no pathconfig in vps.sh"}

[ -z "${_HAVE_LIB_FS}" ]   && source ${_LIB_FS}
[ -z "${_HAVE_LIB_UTIL}" ] && source ${_LIB_UTIL}

vps.loadconfig() {
	[ -z "${VNAME}" ] && util.error "vps.loadconfig: VNAME missing"
	
	: ${VDIRBASE:=${__VDIRBASE}}
	: ${VCONFDIR:=${__PKGCONFDIR}/${VNAME}}
	
	if [ ! -e ${VCONFDIR}/context.conf ]; then
		util.error "vps.loadconfig: cannot find configuration for '${VNAME}'"
	fi
	
	source ${VCONFDIR}/context.conf
	
	# sanity checks
	local needed="VX_XID,VX_INIT"
	
	for i in ${needed}; do
		if [ -z "$(eval echo \$${i})" ]; then
			util.error "vps.loadconfig: missing configuration for ${i}"
		fi
	done
	
	# defaults
	: ${VDIR:=${VDIRBASE}/${VNAME}}
	: ${VX_TIMEOUT_KILL:=30}
	: ${VX_SHELL:=/bin/bash}
	
	if [ ! -d ${VDIR} ]; then
		util.error "vps.loadconfig: cannot find installation for '${VNAME}'"
	fi
}

_VPS_LOCK=
vps.lock() {
	[ -z "${VNAME}" ]     && util.error "vps.lock: VNAME missing"
	[ -n "${_VPS_LOCK}" ] && util.error "vps.lock: lock exists"
	
	# create sync pipe
	local syncpipe=$(mktemp)
	rm -f ${syncpipe}
	mkfifo -m700 ${syncpipe}
	
	# start lock
	${_LOCKFILE} -l ${__PKGLOCKDIR}/${VNAME} -s ${syncpipe} &
	grep -q "true" ${syncpipe} 2>/dev/null
	
	_VPS_LOCK=$!
}

vps.unlock() {
	[ -z "${_VPS_LOCK}" ] && return
	
	kill -HUP ${_VPS_LOCK} >/dev/null || :
	_VPS_LOCK=
	
	return 0
}

vps.locked() {
	[ -f ${__PKGLOCKDIR}/${VNAME} ] || return 1
	
	local pid=$(<${__PKGLOCKDIR}/${VNAME})
	
	[ -d /proc/${pid} ]
}

vps.running() {
	${_VCONTEXT} -I -x ${VX_XID} &>/dev/null || return 1
	return 0
}

vps.context() {
# $1 - subcommand
	local subcmd=$1
	
	[ -z "${subcmd}" ] && util.error "vps.context: missing argument <subcmd>"
	[ -z "${VX_XID}" ] && util.error "vps.context: VX_XID missing"
	
	case ${subcmd} in
		setup)
			${_VNCONTEXT} -C -n ${VX_XID} -f PERSISTANT,INIT_SET
			${_VCONTEXT}  -C -x ${VX_XID} -f PERSISTANT,INIT_SET
			;;
		
		release)
			${_VNFLAGS} -S -n ${VX_XID} -f ~PERSISTANT &>/dev/null || :
			${_VFLAGS}  -S -x ${VX_XID} -f ~PERSISTANT &>/dev/null || :
			;;
		
		*)
			util.error "vps.context: unknown subcommand '${subcmd}'"
	esac
}

vps.network() {
# $1 - subcommand
	local subcmd=$1
	
	[ -z "${subcmd}" ] && util.error "vps.network: missing argument <subcmd>"
	[ -z "${VX_XID}" ] && util.error "vps.network: VX_XID missing"
	
	case ${subcmd} in
		setup)
			for i in ${NX_ADDR[@]}; do
				${_VNCONTEXT} -A -n ${VX_XID} -a ${i}
			done
			;;
		
		*)
			util.error "vps.network: unknown subcommand '${subcmd}'"
	esac
}

vps.namespace() {
# $1 - subcommand
	local subcmd=$1 && shift
	
	[ -z "${subcmd}" ] && util.error "vps.namespace: missing argument <subcmd>"
	[ -z "${VX_XID}" ] && util.error "vps.namespace: VX_XID missing"
	
	case ${subcmd} in
		new)
			${_VNAMESPACE} -N -- \
			${_VNAMESPACE} -S -x ${VX_XID}
			;;
		
		enter)
			[ -z "$1" ] && util.error "vps.namespace: missing argument <command>"
			${_VNAMESPACE} -E -x ${VX_XID} -- "$@"
			;;
		
		*)
			util.error "vps.namespace: unknown subcommand '${subcmd}'"
	esac
}

vps.limit() {
# $1 - subcommand
	local subcmd=$1
	local res limit
	
	[ -z "${subcmd}" ] && util.error "vps.limit: missing argument <subcmd>"
	[ -z "${VX_XID}" ] && util.error "vps.limit: VX_XID missing"
	
	case ${subcmd} in
		set)
			for i in ${VX_LIMIT[@]}; do
				res=${i/=*}
				limit=${i/*=}
				
				${_VLIMIT} -S -r ${res} -l ${limit} -x ${VX_XID}
			done
			;;
		
		*)
			util.error "vps.limit: unknown subcommand '${subcmd}'"
	esac
}

vps.sched() {
# $1 - subcommand
	local subcmd=$1
	
	[ -z "${subcmd}" ] && util.error "vps.sched: missing argument <subcmd>"
	[ -z "${VX_XID}" ] && util.error "vps.sched: VX_XID missing"
	
	case ${subcmd} in
		set)
			[ -z "${VX_SCHED}" ] && return 0
			
			${_VSCHED} -S -b $(util.array_to_list ${VX_SCHED[@]}) -x ${VX_XID}
			;;
		
		*)
			util.error "vps.sched: unknown subcommand '${subcmd}'"
	esac
}

vps.uname() {
# $1 - subcommand
	local subcmd=$1
	local names
	
	[ -z "${subcmd}" ] && util.error "vps.uname: missing argument <subcmd>"
	[ -z "${VX_XID}" ] && util.error "vps.uname: VX_XID missing"
	[ -z "${VNAME}" ]  && util.error "vps.uname: VNAME missing"
	
	case ${subcmd} in
		set)
			${_VUNAME} -S -n CONTEXT=${VNAME} -x ${VX_XID}
			
			[ -z "${VX_UNAME}" ] && return 0
			
			${_VUNAME} -S -n $(util.array_to_list ${VX_UNAME[@]}) -x ${VX_XID}
			;;
		
		*)
			util.error "vps.uname: unknown subcommand '${subcmd}'"
	esac
}

vps.flags() {
# $1 - subcommand
	local subcmd=$1
	local names
	
	[ -z "${subcmd}" ] && util.error "vps.flags: missing argument <subcmd>"
	[ -z "${VX_XID}" ] && util.error "vps.flags: VX_XID missing"
	
	case ${subcmd} in
		set)
			if [ ! -z "${VX_BCAPS}" ]; then
				${_VFLAGS} -S -b $(util.array_to_list ${VX_BCAPS[@]}) -x ${VX_XID}
			fi
			
			if [ ! -z "${VX_CCAPS}" ]; then
				${_VFLAGS} -S -c $(util.array_to_list ${VX_CCAPS[@]}) -x ${VX_XID}
			fi
			
			if [ ! -z "${VX_FLAGS}" ]; then
				${_VFLAGS} -S -f $(util.array_to_list ${VX_FLAGS[@]}) -x ${VX_XID}
			fi
			;;
		
		*)
			util.error "vps.flags: unknown subcommand '${subcmd}'"
	esac
}

vps.init() {
	[ -z "${VX_INIT}" ] && util.error "vps.init: VX_INIT missing"
	[ -z "${VX_XID}" ]  && util.error "vps.init: VX_XID missing"
	[ -z "${VDIR}" ]    && util.error "vps.init: VDIR missing"
	
	pushd ${VDIR} >/dev/null
	
	case ${VX_INIT} in
		plain)
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -cfi -n ${VX_XID} -x ${VX_XID} -- /sbin/init
			;;
		
		gentoo)
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/rc default
			;;
		
		initng)
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/initng
			;;
		
		*)
			util.error "vps.init: unknown init style"
			;;
	esac
	
	popd >/dev/null
}

vps.halt() {
	[ -z "${VX_INIT}" ] && util.error "vps.halt: VX_INIT missing"
	[ -z "${VX_XID}" ]  && util.error "vps.halt: VX_XID missing"
	[ -z "${VDIR}" ]    && util.error "vps.halt: VDIR missing"
	
	#${_VFLAGS} -S -x ${VX_XID} -f REBOOT_KILL
	
	pushd ${VDIR} >/dev/null
	
	case ${VX_INIT} in
		plain)
			${_VFLAGS} -S -f REBOOT_KILL -x ${VX_XID}
			
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/shutdown -h now
			;;
		
		gentoo)
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/rc shutdown
			;;
		
		initng)
			${_VFLAGS} -S -f REBOOT_KILL -x ${VX_XID}
			
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/ngc -0
			;;
		
		*)
			util.error "vps.halt: unknown init style"
			;;
	esac
	
	popd >/dev/null
}

vps.exec() {
	[ -z "${VX_XID}" ]  && util.error "vps.exec: VX_XID missing"
	[ -z "${VDIR}" ]    && util.error "vps.exec: VDIR missing"
	
	pushd ${VDIR} >/dev/null
	${_VNAMESPACE} -E -x ${VX_XID} -- \
	${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- "$@"
	popd >/dev/null
}

vps.login() {
	[ -z "${VX_XID}" ]  && util.error "vps.login: VX_XID missing"
	[ -z "${VDIR}" ]    && util.error "vps.login: VDIR missing"
	
	pushd ${VDIR} >/dev/null
	${_VNAMESPACE} -E -x ${VX_XID} -- \
	${_VLOGIN} -n ${VX_XID} -x ${VX_XID} -- ${VX_SHELL}
	popd >/dev/null
}

vps.kill() {
	[ -z "${VX_XID}" ]  && util.error "vps.kill: VX_XID missing"
	
	${_VKILL} -x ${VX_XID}
}

vps.mount() {
# $1 - mount rootfs (optional)
	local fstab mtab
	
	fstab=$(fs.best_file ${VCONFDIR}/init/fstab \
	                     ${__PKGCONFDIR}/.defaults/init/fstab \
	                     ${__PKGDATADEFAULTSDIR}/fstab)
	
	mtab=$(fs.best_file ${VCONFDIR}/init/mtab \
	                    ${__PKGCONFDIR}/.defaults/init/mtab \
	                    ${__PKGDATADEFAULTSDIR}/mtab)
	
	[ -z "${VDIR}" ]   && util.error "vps.mount: VDIR missing"
	[ -z "${VX_XID}" ] && util.error "vps.mount: VX_XID missing"
	
	[ -z "${fstab}" ] && util.error "vps.mount: cannot find fstab"
	[ -z "${mtab}" ]  && util.error "vps.mount: cannot find mtab"
	
	if [ "$1" == "root" ]; then
		${_VNAMESPACE} -E -x ${VX_XID} -- \
		${_VMOUNT} -M -n -r ${VDIR}
	else
		pushd ${VDIR} >/dev/null
		
		if [ -n "${mtab}" ]; then
			cp ${mtab} etc/mtab
		else
			: > etc/mtab
		fi
		
		${_VNAMESPACE} -E -x ${VX_XID} -- \
		${_VMOUNT} -M -f ${fstab} -m etc/mtab
		
		popd >/dev/null
	fi
	
	return 0
}

vps.umount() {
	local fstab
	
	fstab=$(fs.best_file ${VCONFDIR}/init/fstab \
	                     ${__PKGCONFDIR}/.defaults/init/fstab \
	                     ${__PKGDATADEFAULTSDIR}/fstab)
	
	[ -z "${VX_XID}" ] && util.error "vps.umount: VX_XID missing"
	
	[ -z "${fstab}" ] && util.error "vps.umount: cannot find fstab"
	
	pushd ${VDIR} >/dev/null
	
	${_VNAMESPACE} -E -x ${VX_XID} -- \
	${_VMOUNT} -U -f ${fstab} -m etc/mtab
	
	popd >/dev/null
}

_HAVE_LIB_VPS=1
