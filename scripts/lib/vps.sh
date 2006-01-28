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

[ -z "${_HAVE_LIB_UTIL}" ] && source ${_LIB_UTIL}

vps.config() {
# $1 - subcommand
# $2 - configuration key
	[ -z "${VNAME}" ] && util.error "vps.config: VNAME missing"
	
	local subcmd=$1
	local key=$2
	
	[ -z "${subcmd}" ] && util.error "vps.config: missing argument <subcmd>"
	
	case ${subcmd} in
		get)
			[ -z "${key}" ] && util.error "vps.config: missing argument <key>"
			${_VCONFIG} -G -n ${VNAME} -k ${key}
			;;
		
		list)
			if [ -z "${key}" ]; then
				${_VCONFIG} -L
			else
				${_VCONFIG} -L | grep "^${key}"
			fi
			;;
		
		*)
			util.error "vps.config: unknown subcommand '${subcmd}'"
	esac
}

vps.loadconfig() {
	[ -z "${VNAME}" ] && util.error "vps.loadconfig: VNAME missing"
	
	# sanity checks
	VDIR=$(vps.config get namespace.root)
	VX_INIT=$(vps.config get vps.init)
	VX_XID=$(vps.config get context.id)
	
	: ${VDIR:=${__VDIRBASE}/${VNAME}}
	
	[ -z "${VX_INIT}" ] && util.error "vps.loadconfig: missing configuration for vps.init"
	[ -z "${VX_XID}" ] && util.error "vps.loadconfig: missing configuration for context.id"
	
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
			vps.config get net.addr | while read line; do
				${_VNCONTEXT} -A -n ${VX_XID} -a ${line}
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
			${_VNAMESPACE} -N -x ${VX_XID}
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
			for i in $(vps.config list limit.); do
				res=${i/limit.}
				limit=$(vps.config get ${i})
				
				[ -z "${limit}" ] && continue
				
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
	local sched
	
	[ -z "${subcmd}" ] && util.error "vps.sched: missing argument <subcmd>"
	[ -z "${VX_XID}" ] && util.error "vps.sched: VX_XID missing"
	
	case ${subcmd} in
		set)
			sched=$(vps.config get context.sched)
			
			[ -z "${sched}" ] && return 0
			
			${_VSCHED} -S -b ${sched} -x ${VX_XID}
			;;
		
		*)
			util.error "vps.sched: unknown subcommand '${subcmd}'"
	esac
}

vps.uname() {
# $1 - subcommand
	local subcmd=$1
	local uts name
	
	[ -z "${subcmd}" ] && util.error "vps.uname: missing argument <subcmd>"
	[ -z "${VX_XID}" ] && util.error "vps.uname: VX_XID missing"
	[ -z "${VNAME}" ]  && util.error "vps.uname: VNAME missing"
	
	case ${subcmd} in
		set)
			${_VUNAME} -S -n CONTEXT=${VNAME} -x ${VX_XID}
			
			for i in $(vps.config list uts.); do
				uts=${i/uts.}
				name=$(vps.config get ${i})
				
				[ -z "${name}" ] && continue
				
				${_VUNAME} -S -n ${uts}=${name} -x ${VX_XID}
			done
			;;
		
		*)
			util.error "vps.uname: unknown subcommand '${subcmd}'"
	esac
}

vps.flags() {
# $1 - subcommand
	local subcmd=$1
	local bcaps ccaps flags
	
	[ -z "${subcmd}" ] && util.error "vps.flags: missing argument <subcmd>"
	[ -z "${VX_XID}" ] && util.error "vps.flags: VX_XID missing"
	
	case ${subcmd} in
		set)
			bcaps=$(vps.config get context.bcapabilities)
			ccaps=$(vps.config get context.ccapabilities)
			flags=$(vps.config get context.flags)
			
			if [ ! -z "${bcaps}" ]; then
				${_VFLAGS} -S -b ${bcaps} -x ${VX_XID}
			fi
			
			if [ ! -z "${ccaps}" ]; then
				${_VFLAGS} -S -c ${ccaps} -x ${VX_XID}
			fi
			
			if [ ! -z "${flags}" ]; then
				${_VFLAGS} -S -f ${flags} -x ${VX_XID}
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
		sysvinit)
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -cfi -n ${VX_XID} -x ${VX_XID} -- /sbin/init
			;;
		
		sysvrc)
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /etc/rc.d/rc 3
			;;
		
		initng)
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -cfi -n ${VX_XID} -x ${VX_XID} -- /sbin/initng
			;;
		
		gentoo)
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/rc sysinit
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/rc boot
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/rc default
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
	
	pushd ${VDIR} >/dev/null
	
	case ${VX_INIT} in
		sysvinit)
			${_VFLAGS} -S -f REBOOT_KILL -x ${VX_XID}
			
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/shutdown -h now
			;;
		
		sysvrc)
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /etc/rc.d/rc 0
			;;
		
		initng)
			${_VFLAGS} -S -f REBOOT_KILL -x ${VX_XID}
			
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/ngc -0
			;;
		
		gentoo)
			${_VNAMESPACE} -E -x ${VX_XID} -- \
			${_VEXEC} -c -n ${VX_XID} -x ${VX_XID} -- /sbin/rc shutdown
			;;
		
		*)
			util.error "vps.halt: unknown init style"
			;;
	esac
	
	${_VNFLAGS} -S -f ~PERSISTANT -n ${VX_XID} || :
	${_VFLAGS}  -S -f ~PERSISTANT -x ${VX_XID} || :
	
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
	
	local shell=$(vps.config get vps.shell)
	
	: ${shell:=/bin/bash}
	
	pushd ${VDIR} >/dev/null
	${_VNAMESPACE} -E -x ${VX_XID} -- \
	${_VLOGIN} -n ${VX_XID} -x ${VX_XID} -- ${shell}
	popd >/dev/null
}

vps.kill() {
	[ -z "${VX_XID}" ]  && util.error "vps.kill: VX_XID missing"
	
	${_VKILL} -x ${VX_XID}
}

vps.mount() {
# $1 - mount rootfs (optional)
	local fstab=$(vps.config get namespace.fstab)
	local mtab=$(vps.config get namespace.mtab)
	
	fstab_tmp=$(mktemp)
	mtab_tmp=$(mktemp)
	
	if [ -n "${fstab}" ]; then
		echo ${fstab} > ${fstab_tmp}
	else
		cat ${__PKGCONFDIR}/fstab > ${fstab_tmp}
	fi
	
	if [ -n "${mtab}" ]; then
		echo ${mtab} > ${fstab_tmp}
	else
		cat ${__PKGCONFDIR}/mtab > ${mtab_tmp}
	fi
	
	[ -z "${VDIR}" ]   && util.error "vps.mount: VDIR missing"
	[ -z "${VX_XID}" ] && util.error "vps.mount: VX_XID missing"
	
	if [ "$1" == "root" ]; then
		${_VNAMESPACE} -E -x ${VX_XID} -- \
		${_VMOUNT} -M -n -r ${VDIR}
	else
		pushd ${VDIR} >/dev/null
		
		cp ${mtab_tmp} etc/mtab
		
		${_VNAMESPACE} -E -x ${VX_XID} -- \
		${_VMOUNT} -M -f ${fstab_tmp} -m etc/mtab
		
		popd >/dev/null
	fi
	
	return 0
}

vps.umount() {
	[ -z "${VDIR}" ]   && util.error "vps.mount: VDIR missing"
	[ -z "${VX_XID}" ] && util.error "vps.umount: VX_XID missing"
	
	pushd ${VDIR} >/dev/null
	${_VNAMESPACE} -E -x ${VX_XID} -- \
	${_VMOUNT} -U -m etc/mtab
	popd >/dev/null
}

_HAVE_LIB_VPS=1
