// Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <vserver.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include "lucid.h"
#include "xmlrpc.h"

#include "auth.h"
#include "cfg.h"
#include "lists.h"
#include "log.h"
#include "methods.h"
#include "vxdb.h"

#ifndef MS_REC
#define MS_REC 16384
#endif

#ifndef CLONE_NEWNS
#define CLONE_NEWNS 0x00020000
#endif

/* start process:
   1) setup context
   1.1) create new context
   1.2) setup capabilities
   1.3) setup resource limits
   1.4) setup scheduler
   1.5) setup unames
   2) setup network context
   2.1) create new network context
   2.2) add ip addresses
   3) setup filesystem namespace
   3.1) create new namespace
   3.2) mount fstab entries
   3.3) remount root filesystem
   4) setup disk limits
   4.1) set context id on all files
   4.2) calculate size of all files
   4.3) set disk limit
   5) guest init
   5.1) start guest init/rc scripts
*/

static char *name = NULL;
static xid_t xid = 0;
static char *errmsg = NULL;

static
char *vhifields[] = {
	"domainname",
	"machine",
	"nodename",
	"release",
	"sysname",
	"version",
	NULL
};

static
int setup_context(void)
{
	dbi_result dbr;
	uint32_t buf32;
	char *buf;
	int i;
	
	struct vx_create_flags create_flags = {
		.flags = VXF_PERSISTENT,
	};
	
	struct vx_caps caps = {
		.bcaps = -1,
		.ccaps = 0,
		.cmask = 0,
	};
	
	struct vx_flags cflags = {
		.flags = 0,
		.mask  = 0,
	};
	
	struct vx_rlimit_mask rlimit_mask = {
		.minimum   = 0,
		.softlimit = 0,
		.maximum   = 0,
	};
	
	struct vx_rlimit rlimit = {
		.id        = -1,
		.minimum   = 0,
		.softlimit = 0,
		.maximum   = 0,
	};
	
	struct vx_sched sched = {
		.set_mask = 0,
		.fill_rate = 0,
		.interval = 0,
		.tokens = 0,
		.tokens_min = 0,
		.tokens_max = 0,
		.prio_bias = 0,
		.cpu_id = 0,
		.bucket_id = 0,
	};
	
	struct vx_vhi_name vhiname = {
		.field = 0,
		.name  = "",
	};
	
	int status;
	pid_t pid = fork();
	
	switch (pid) {
	case -1:
		return errno = MESYS, -1;
	
	case 0:
		usleep(200);
		
		if (vx_create(xid, &create_flags) == -1)
			exit(EXIT_FAILURE);
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			return errno = MESYS, -1;
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			return errno = MESYS, -1;
		
		if (WIFSIGNALED(status))
			kill(getpid(), WTERMSIG(status));
	}
	
	uint64_t mask; // dummy
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT bcap FROM vx_bcaps WHERE xid = %d",
		xid);
	
	if (!dbr)
		return errno = MEVXDB, -1;
	
	while (dbi_result_next_row(dbr)) {
		uint64_t bcap;
		flist64_getval(bcaps_list, (char *) dbi_result_get_string(dbr, "bcap"), &bcap);
		caps.bcaps &= ~bcap;
	}
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT ccap FROM vx_ccaps WHERE xid = %d",
		xid);
	
	if (!dbr)
		return errno = MEVXDB, -1;
	
	while (dbi_result_next_row(dbr)) {
		uint64_t ccap;
		flist64_getval(ccaps_list, (char *) dbi_result_get_string(dbr, "ccap"), &ccap);
		caps.ccaps |= ccap;
		caps.cmask  = caps.ccaps;
	}
	
	if (vx_set_caps(xid, &caps) == -1)
		return errno = MESYS, -1;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT flag FROM vx_flags WHERE xid = %d",
		xid);
	
	if (!dbr)
		return errno = MEVXDB, -1;
	
	while (dbi_result_next_row(dbr)) {
		uint64_t flag;
		flist64_getval(cflags_list, (char *) dbi_result_get_string(dbr, "flag"), &flag);
		cflags.flags |= flag;
		cflags.mask   = cflags.flags;
	}
	
	if (vx_set_flags(xid, &cflags) == -1)
		return errno = MESYS, -1;
	
	if (vx_get_rlimit_mask(&rlimit_mask) == -1)
		return errno = MESYS, -1;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT type,min,soft,max FROM vx_limit WHERE xid = %d",
		xid);
	
	if (!dbr)
		return errno = MEVXDB, -1;
	
	while (dbi_result_next_row(dbr)) {
		char *type = (char *) dbi_result_get_string(dbr, "type");
		
		if (flist32_getval(rlimit_list, type, &buf32) == -1)
			continue;
		
		if ((rlimit_mask.minimum   & buf32) != buf32 &&
		    (rlimit_mask.softlimit & buf32) != buf32 &&
		    (rlimit_mask.maximum   & buf32) != buf32)
			continue;
		
		rlimit.id        = flist32_mask2val(buf32);
		rlimit.minimum   = dbi_result_get_longlong(dbr, "min");
		rlimit.softlimit = dbi_result_get_longlong(dbr, "soft");
		rlimit.maximum   = dbi_result_get_longlong(dbr, "max");
		
		if (vx_set_rlimit(xid, &rlimit) == -1)
			return errno = MESYS, -1;
	}
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT fill_rate,interval,prio_bias,tokens_min,tokens_max "
		"FROM vx_sched WHERE xid = %d",
		xid);
	
	if (!dbr)
		return errno = MEVXDB, -1;
	
	if (dbi_result_get_numrows(dbr) > 0) {
		dbi_result_first_row(dbr);
		sched.fill_rate  = dbi_result_get_int(dbr, "fill_rate");
		sched.interval   = dbi_result_get_int(dbr, "interval");
		sched.prio_bias  = dbi_result_get_int(dbr, "prio_bias");
		sched.tokens_min = dbi_result_get_int(dbr, "tokens_min");
		sched.tokens_max = dbi_result_get_int(dbr, "tokens_max");
		
		if (sched.fill_rate == 0 ||
				sched.interval == 0 ||
				sched.tokens_max == 0)
			return errno = MECONF, -1;
		
		if (sched.fill_rate > sched.interval)
			sched.fill_rate = sched.interval;
		
		if (sched.tokens_max < sched.fill_rate)
			sched.tokens_max = sched.fill_rate;
		
		if (sched.tokens_min > sched.tokens_max)
			sched.tokens_min = sched.tokens_max;
		
		sched.set_mask |= VXSM_FILL_RATE|VXSM_INTERVAL|VXSM_TOKENS;
		sched.set_mask |= VXSM_TOKENS_MIN|VXSM_TOKENS_MAX|VXSM_PRIO_BIAS;
		sched.tokens = sched.tokens_max;
		
		if (vx_set_sched(xid, &sched) == -1)
			return errno = MESYS, -1;
	}
	
	for (i = 0; vhifields[i]; i++) {
		dbr = dbi_conn_queryf(vxdb,
			"SELECT %s FROM vx_uname WHERE xid = %d",
			vhifields[i], xid);
		
		if (!dbr)
			return errno = MEVXDB, -1;
		
		if (dbi_result_get_numrows(dbr) < 1)
			continue;
		
		if (flist32_getval(vhiname_list, vhifields[i], &buf32) == -1)
			continue;
		
		vhiname.field = flist32_mask2val(buf32);
		
		dbi_result_first_row(dbr);
		buf = (char *) dbi_result_get_string(dbr, vhifields[i]);
		
		if(!buf || !*buf)
			continue;
		
		bzero(vhiname.name, VHILEN);
		strncpy(vhiname.name, buf, VHILEN-1);
		
		if (vx_set_vhi_name(xid, &vhiname) == -1)
			return errno = MESYS, -1;
	}
	
	if (name) {
		vhiname.field = VHIN_CONTEXT;
		
		strncpy(vhiname.name, name, VHILEN-1);
		vhiname.name[VHILEN-1] = '\0';
		
		if (vx_set_vhi_name(xid, &vhiname) == -1)
			return errno = MESYS, -1;
	}
	
	return 0;
}

static
int setup_network(void)
{
	dbi_result dbr;
	uint32_t buf32;
	char *buf;
	int i;
	
	struct nx_create_flags create_flags = {
		.flags = NXF_PERSISTENT,
	};
	
	struct nx_flags flags = {
		.flags = 0,
		.mask  = 0,
	};
	
	struct nx_addr addr;
	
	int status;
	pid_t pid = fork();
	
	switch (pid) {
	case -1:
		return errno = MESYS, -1;
	
	case 0:
		usleep(200);
		
		if (nx_create(xid, &create_flags) == -1)
			exit(EXIT_FAILURE);
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			return errno = MESYS, -1;
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			return errno = MESYS, -1;
		
		if (WIFSIGNALED(status))
			kill(getpid(), WTERMSIG(status));
	}
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT addr,netmask FROM nx_addr WHERE xid = %d",
		xid);
	
	if (!dbr)
		return errno = MEVXDB, -1;
	
	while (dbi_result_next_row(dbr)) {
		char *ip   = (char *) dbi_result_get_string(dbr, "addr");
		char *netm = (char *) dbi_result_get_string(dbr, "netmask");
		asprintf(&buf, "%s/%s", ip, netm);
		
		addr.type  = NXA_TYPE_IPV4;
		addr.count = 1;
		
		if (addr_from_str(buf, &addr.ip[0], &addr.mask[0]) == -1)
			return errno = MECONF, -1;
		
		free(buf);
		
		if (nx_add_addr(xid, &addr) == -1)
			return errno = MESYS, -1;
	}
	
	return 0;
}

static
int mount_namespace(void)
{
	dbi_result dbr;
	int mtabfd;
	
	char *vdirbase = cfg_getstr(cfg, "vserver-dir");
	char *vdir = NULL;
	
	asprintf(&vdir, "%s/%s", vdirbase, name);
	
	if (chroot_secure_chdir(vdir, "/etc") == -1)
		return errno = MESYS, -1;
	
	if ((mtabfd = open_trunc("mtab")) == -1)
		return errno = MESYS, -1;
	
	if (write(mtabfd, "/dev/hdv1 / ufs rw 0 0\n", 23) == -1)
		return errno = MESYS, -1;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT spec,file,vfstype,mntops FROM init_mount WHERE xid = %d",
		xid);
	
	if (!dbr)
		return errno = MEVXDB, -1;
	
	while (dbi_result_next_row(dbr)) {
		char *src  = (char *) dbi_result_get_string(dbr, "spec");
		char *dst  = (char *) dbi_result_get_string(dbr, "file");
		char *type = (char *) dbi_result_get_string(dbr, "vfstype");
		char *opts = (char *) dbi_result_get_string(dbr, "mntops");
		
		if (!type || !*type)
			type = "auto";
		
		if (!opts || !*opts)
			opts = "defaults";
		
		if (chroot_secure_chdir(vdir, dst) == -1)
			return errno = MESYS, -1;
		
		char *cmd;
		asprintf(&cmd, "mount -n -t %s -o %s %s .", type, opts, src);
		
		if (exec_fork(cmd) != 0)
			return errno = MESYS, -1;
		
		dprintf(mtabfd, "%s %s %s %s 0 0\n", src, dst, type, opts);
	}
	
	if (chroot_secure_chdir(vdir, "/") == -1)
		return errno = MESYS, -1;
	
	if (mount(".", "/", NULL, MS_BIND|MS_REC, NULL) == -1)
		return errno = MESYS, -1;
	
	close(mtabfd);
	return 0;
}

static
int setup_namespace(void)
{
	int status;
	pid_t pid = sys_clone(CLONE_NEWNS|SIGCHLD, 0);
	
	switch (pid) {
	case -1:
		return errno = MESYS, -1;
	
	case 0:
		usleep(200);
		
		if (mount_namespace() == -1)
			exit(errno);
		 
		if (vx_set_namespace(xid) == -1)
			exit(MESYS);
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			return errno = MESYS, -1;
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			return errno = WEXITSTATUS(status), -1;
		
		if (WIFSIGNALED(status))
			kill(getpid(), WTERMSIG(status));
	}
	
	return 0;
}

static
int setup_disklimit(void)
{
	return 0;
}

static
int guest_init(void)
{
	dbi_result dbr;
	char *method, *start;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT method,start FROM init_method WHERE xid = %d",
		xid);
	
	if (!dbr)
		return errno = MEVXDB, -1;
	
	if (dbi_result_get_numrows(dbr) < 1) {
		method = "init";
		start = "";
	}
	
	else {
		dbi_result_first_row(dbr);
		method = (char *) dbi_result_get_string(dbr, "method");
		start  = (char *) dbi_result_get_string(dbr, "start");
	}
	
	pid_t pid;
	int i, status, waitchild = 1;
	
	struct vx_flags cflags = {
		.flags = 0,
		.mask = 0,
	};
	
	struct vx_migrate_flags migrate_flags = {
		.flags = 0,
	};
	
	char *vdirbase = cfg_getstr(cfg, "vserver-dir");
	char *vdir = NULL;
	
	asprintf(&vdir, "%s/%s", vdirbase, name);
	
	if (strcmp(method, "init") == 0) {
		migrate_flags.flags = VXM_SET_INIT|VXM_SET_REAPER;
		cflags.flags = cflags.mask = VXF_INFO_INIT;
		waitchild = 0;
	}
	
	if (vx_set_flags(xid, &cflags) == -1)
		return errno = MESYS, -1;
	
	switch ((pid = fork())) {
	case -1:
		return errno = MESYS, -1;
	
	case 0:
		for (i = 0; i < 100; i++)
			close(i);
		
		if (vx_enter_namespace(xid) == -1 ||
		    chroot(vdir) == -1 ||
		    nx_migrate(xid) == -1 ||
		    vx_migrate(xid, &migrate_flags) == -1)
			exit(EXIT_FAILURE);
		
		if (strcmp(method, "init") == 0) {
			if (exec_replace("/sbin/init") == -1)
				exit(EXIT_FAILURE);
		}
		
		else if (strcmp(method, "gentoo") == 0) {
			if (!start || !*start)
				start = "default";
			
			exec_fork("/sbin/rc sysinit");
			exec_fork("/sbin/rc boot");
			exec_fork("/sbin/rc %s", start); /* /sbin/rc always fails */
			exit(EXIT_SUCCESS);
		}
		
		else
			exit(EXIT_FAILURE);
		
		exit(EXIT_SUCCESS);
	
	default:
		switch (waitpid(pid, &status, waitchild == 0 ? WNOHANG : 0)) {
		case -1:
			return errno = MESYS, -1;
		
		case 0:
			return 0;
		
		default:
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				return errno = MESYS, -1;
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
		}
	}
	
	return 0;
}

static
void cleanup_on_exit(void)
{
	struct vx_flags vflags = {
		.flags = 0,
		.mask  = VXF_PERSISTENT,
	};
	
	struct nx_flags nflags = {
		.flags = 0,
		.mask  = NXF_PERSISTENT,
	};
	
	vx_set_flags(xid, &vflags);
	nx_set_flags(xid, &nflags);
}

/* vx.start(string name) */
XMLRPC_VALUE m_vx_start(XMLRPC_SERVER s, XMLRPC_REQUEST r, void *d)
{
	dbi_result dbr;
	XMLRPC_VALUE params = method_get_params(r);
	
	if (!auth_isadmin(r) && !auth_isowner(r))
		return method_error(MEPERM);
	
	name = XMLRPC_VectorGetStringWithID(params, "name");
	
	if (!name)
		return method_error(MEREQ);
	
	if (vxdb_getxid(name, &xid) == -1)
		return method_error(MENOENT);
	
	if (vx_get_info(xid, NULL) != -1)
		return method_error(MERUNNING);
	
	if (setup_context() == -1 ||
	    setup_network() == -1 ||
	    setup_namespace() == -1 ||
	    setup_disklimit() == -1 ||
	    guest_init() == -1) {
		cleanup_on_exit();
		m_vx_stop(s, r, d);
		return method_error(errno);
	}
	
	cleanup_on_exit();
	return params;
}
