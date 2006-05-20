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
		return method_error(&errmsg, "fork: %s", strerror(errno));
	
	case 0:
		usleep(200);
		
		if (vx_create(xid, &create_flags) == -1) {
			log_debug("vx_create(%d): %s", xid, strerror(errno));
			exit(EXIT_FAILURE);
		}
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			return method_error(&errmsg, "waitpid1: %s", strerror(errno));
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			return method_error(&errmsg, "context creation failed");
		
		if (WIFSIGNALED(status))
			kill(getpid(), WTERMSIG(status));
	}
	
	uint64_t mask; // dummy
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT bcap FROM vx_bcaps WHERE xid = %d",
		xid);
	
	if (!dbr)
		goto ccaps;
	
	while (dbi_result_next_row(dbr)) {
		uint64_t bcap;
		flist64_getval(bcaps_list, (char *) dbi_result_get_string(dbr, "bcap"), &bcap);
		caps.bcaps &= ~bcap;
	}
	
ccaps:
	dbr = dbi_conn_queryf(vxdb,
		"SELECT ccap FROM vx_ccaps WHERE xid = %d",
		xid);
	
	if (!dbr)
		goto cflags;
	
	while (dbi_result_next_row(dbr)) {
		flist64_parse((char *) dbi_result_get_string(dbr, "ccap"),
		              ccaps_list, &caps.ccaps, &caps.cmask, '~', ',');
	}
	
	if (vx_set_caps(xid, &caps) == -1)
		return method_error(&errmsg, "vx_set_caps: %s", strerror(errno));
	
cflags:
	dbr = dbi_conn_queryf(vxdb,
		"SELECT flag FROM vx_flags WHERE xid = %d",
		xid);
	
	if (!dbr)
		goto rlimits;
	
	while (dbi_result_next_row(dbr)) {
		flist64_parse((char *) dbi_result_get_string(dbr, "flag"),
		              cflags_list, &cflags.flags, &cflags.mask, '~', ',');
	}
	
	if (vx_set_flags(xid, &cflags) == -1)
		return method_error(&errmsg, "vx_set_flags: %s", strerror(errno));
	
rlimits:
	if (vx_get_rlimit_mask(&rlimit_mask) == -1)
		return method_error(&errmsg, "vx_get_rlimit_mask: %s", strerror(errno));
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT type,min,soft,max FROM vx_limit WHERE xid = %d",
		xid);
	
	if (!dbr)
		goto sched;
	
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
			return method_error(&errmsg, "vx_get_rlimit_mask: %s", strerror(errno));
	}
	
sched:
	dbr = dbi_conn_queryf(vxdb,
		"SELECT fill_rate,interval,prio_bias,tokens_min,tokens_max "
		"FROM vx_sched WHERE xid = %d",
		xid);
	
	if (!dbr && dbi_result_get_numrows(dbr) < 1)
		goto uname;
	
	dbi_result_first_row(dbr);
	sched.fill_rate  = dbi_result_get_int(dbr, "fill_rate");
	sched.interval   = dbi_result_get_int(dbr, "interval");
	sched.prio_bias  = dbi_result_get_int(dbr, "prio_bias");
	sched.tokens_min = dbi_result_get_int(dbr, "tokens_min");
	sched.tokens_max = dbi_result_get_int(dbr, "tokens_max");
	
	if (sched.fill_rate == 0 ||
	    sched.interval == 0 ||
	    sched.tokens_max == 0)
		goto uname;
	
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
		return method_error(&errmsg, "vx_set_sched: %s", strerror(errno));
	
uname:
	for (i = 0; vhifields[i]; i++) {
		dbr = dbi_conn_queryf(vxdb,
			"SELECT %s FROM vx_uname WHERE xid = %d",
			vhifields[i], xid);
		
		if (!dbr || flist32_getval(vhiname_list, vhifields[i], &buf32))
			continue;
		
		vhiname.field = flist32_mask2val(buf32);
		
		dbi_result_first_row(dbr);
		buf = (char *) dbi_result_get_string(dbr, vhifields[i]);
		
		if(!buf || !*buf)
			continue;
		
		bzero(vhiname.name, VHILEN);
		strncpy(vhiname.name, buf, VHILEN-1);
		
		if (vx_set_vhi_name(xid, &vhiname) == -1) \
			return method_error(&errmsg, "vx_set_vhi_name: %s", strerror(errno));
	}
	
	if (name) {
		vhiname.field = VHIN_CONTEXT;
		
		strncpy(vhiname.name, name, VHILEN-1);
		vhiname.name[VHILEN-1] = '\0';
		
		if (vx_set_vhi_name(xid, &vhiname) == -1)
			return method_error(&errmsg, "vx_set_vhi_name: %s", strerror(errno));
	}
	
	return 0;
}

static
int str_to_addr(char *str, uint32_t *ip, uint32_t *mask)
{
	struct in_addr ib;
	char *addr_ip, *addr_mask;
	
	*ip   = 0;
	*mask = 0;
	
	addr_ip   = strtok(str, "/");
	addr_mask = strtok(NULL, "/");
	
	if (addr_ip == 0)
		return -1;
	
	if (inet_aton(addr_ip, &ib) == -1)
		return -1;
	
	*ip = ib.s_addr;
	
	if (addr_mask == 0) {
		/* default to /24 */
		*mask = ntohl(0xffffff00);
	} else {
		if (strchr(addr_mask, '.') == 0) {
			/* We have CIDR notation */
			int sz = atoi(addr_mask);
			
			for (*mask = 0; sz > 0; --sz) {
				*mask >>= 1;
				*mask  |= 0x80000000;
			}
			
			*mask = ntohl(*mask);
		} else {
			/* Standard netmask notation */
			if (inet_aton(addr_mask, &ib) == -1)
				return -1;
			
			*mask = ib.s_addr;
		}
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
		return method_error(&errmsg, "fork: %s", strerror(errno));
	
	case 0:
		usleep(200);
		
		if (nx_create(xid, &create_flags) == -1) {
			method_error(&errmsg, "nx_create: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			return method_error(&errmsg, "waitpid2: %s", strerror(errno));
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			return method_error(&errmsg, "network context creation failed");
		
		if (WIFSIGNALED(status))
			kill(getpid(), WTERMSIG(status));
	}
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT addr,netmask FROM nx_addr WHERE xid = %d",
		xid);
	
	if (!dbr || dbi_result_get_numrows(dbr) < 1)
		return 0;
	
	while (dbi_result_next_row(dbr)) {
		char *ip   = (char *) dbi_result_get_string(dbr, "addr");
		char *netm = (char *) dbi_result_get_string(dbr, "netmask");
		asprintf(&buf, "%s/%s", ip, netm);
		
		addr.type  = NXA_TYPE_IPV4;
		addr.count = 1;
		
		if (str_to_addr(buf, &addr.ip[0], &addr.mask[0]) == -1)
			return method_error(&errmsg, "str_to_addr: %s", strerror(errno));
		
		if (nx_add_addr(xid, &addr) == -1)
			return method_error(&errmsg, "nx_add_addr: %s", strerror(errno));
	}
	
	return 0;
}

static
int fchroot(int fd)
{
	if (fchdir(fd) == -1)
		return -1;
	
	if (chroot(".") == -1)
		return -1;
	
	return 0;
}

/* go to <dir> in <cwd> as root, while <root> is the original root.
** going into the chroot before doing chdir(dir) prevents symlink attacks
** and hence is safer */
static
int secure_chdir(int rootfd, int cwdfd, char *dir)
{
	int dirfd;
	int errno_orig;
	
	/* check cwdfd */
	if (fchroot(cwdfd) == -1)
		return -1;
	
	/* now go to dir in the chroot */
	if (chdir(dir) == -1)
		goto err;
	
	/* save a file descriptor of the target dir */
	dirfd = open(".", O_RDONLY|O_DIRECTORY);
	
	if (dirfd == -1)
		goto err;
	
	/* break out of the chroot */
	fchroot(rootfd);
	
	/* now go to the saved target dir (but outside the chroot) */
	if (fchdir(dirfd) == -1)
		goto err2;
	
	close(dirfd);
	return 0;
	
err2:
	errno_orig = errno;
	close(dirfd);
	errno = errno_orig;
err:
	errno_orig = errno;
	fchroot(rootfd);
	errno = errno_orig;
	return -1;
}

static
int mount_namespace(void)
{
	int rootfd;
	int vdirfd;
	int mtabfd;
	char *vdirbase = cfg_getstr(cfg, "vserver-basedir");
	char *vdir = NULL;
	dbi_result dbr;
	
	asprintf(&vdir, "%s/%s", vdirbase, name);
	
	if ((rootfd = open("/",  O_RDONLY|O_DIRECTORY)) == -1 ||
	    (vdirfd = open(vdir, O_RDONLY|O_DIRECTORY)) == -1 ||
	    secure_chdir(rootfd, vdirfd, "/etc") == -1 ||
	    (mtabfd = open_trunc("mtab")) == -1 ||
	    write(mtabfd, "/dev/hdv1 / ufs rw 0 0\n", 23) == -1)
		return -1;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT spec,file,vfstype,mntops FROM init_mount WHERE xid = %d",
		xid);
	
	if (!dbr)
		return -1;
	
	while (dbi_result_next_row(dbr)) {
		char *src  = (char *) dbi_result_get_string(dbr, "spec");
		char *dst  = (char *) dbi_result_get_string(dbr, "file");
		char *type = (char *) dbi_result_get_string(dbr, "vfstype");
		char *opts = (char *) dbi_result_get_string(dbr, "mntops");
		
		if (!type || !*type)
			type = "auto";
		
		if (!opts || !*opts)
			opts = "defaults";
		
		if (secure_chdir(rootfd, vdirfd, dst) == -1)
			return -1;
		
		pid_t pid;
		int status;
		char *cmd;
		
		switch((pid = fork())) {
		case -1:
			return -1;
		
		case 0:
			usleep(200);
			
			asprintf(&cmd, "mount -n -t '%s' -o '%s' '%s' .", type, opts, src);
			
			if (system(cmd) == -1)
				exit(EXIT_FAILURE);
			
			exit(EXIT_SUCCESS);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				return -1;
			
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				return -1;
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
		}
		
		dprintf(mtabfd, "%s %s %s %s 0 0\n", src, dst, type, opts);
	}
	
	if (secure_chdir(rootfd, vdirfd, "/") == -1 ||
	    mount(".", "/", NULL, MS_BIND|MS_REC, NULL) == -1)
		return -1;
	
	close(mtabfd);
	close(vdirfd);
	close(rootfd);
	
	return 0;
}


static
int setup_namespace(void)
{
	int status;
	pid_t pid = sys_clone(CLONE_NEWNS|SIGCHLD, 0);
	
	switch (pid) {
	case -1:
		return method_error(&errmsg, "sys_clone: %s", strerror(errno));
	
	case 0:
		usleep(200);
		
		if (mount_namespace() == -1 || vx_set_namespace(xid) == -1)
			exit(EXIT_FAILURE);
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			return method_error(&errmsg, "waitpid3: %s", strerror(errno));
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			return method_error(&errmsg, "namespace setup failed");
		
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
int init_exec(char *method)
{
	pid_t pid;
	int i, status, waitchild = 1;
	
	struct vx_flags cflags = {
		.flags = 0,
		.mask = 0,
	};
	
	struct vx_migrate_flags migrate_flags = {
		.flags = 0,
	};
	
	char *vdirbase = cfg_getstr(cfg, "vserver-basedir");
	char *vdir = NULL;
	
	asprintf(&vdir, "%s/%s", vdirbase, name);
	
	if (strcmp(method, "init") == 0) {
		migrate_flags.flags = VXM_SET_INIT|VXM_SET_REAPER;
		cflags.flags = cflags.mask = VXF_INFO_INIT;
		waitchild = 0;
	}
	
	switch ((pid = fork())) {
	case -1:
		return method_error(&errmsg, "fork: %s", strerror(errno));
	
	case 0:
		for (i = 0; i < 100; i++)
			close(i);
		
		if (vx_set_flags(xid, &cflags) == -1 ||
		    vx_enter_namespace(xid) == -1 ||
		    chroot(vdir) == -1 ||
		    nx_migrate(xid) == -1 ||
		    vx_migrate(xid, &migrate_flags) == -1)
			exit(EXIT_FAILURE);
		
		if (strcmp(method, "init") == 0)
			execl("/sbin/init", "/sbin/init", NULL);
		else
			exit(EXIT_FAILURE);
		
		exit(EXIT_FAILURE);
	
	default:
		/* to wait or not to wait!? */
		switch (waitpid(pid, &status, waitchild == 0 ? WNOHANG : 0)) {
		case -1:
			return method_error(&errmsg, "waitpid4: %s", strerror(errno));
		
		case 0:
			break;
		
		default:
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				return method_error(&errmsg, "init command failed");
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
		}
	}
	
	return 0;
}

static
int guest_init(void)
{
	dbi_result dbr;
	int ac;
	char *method, *start, **av;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT method,start FROM init_method WHERE xid = %d",
		xid);
	
	if (!dbr || dbi_result_get_numrows(dbr) < 1) {
		method = "init";
		start = "";
	}
	
	else {
		dbi_result_first_row(dbr);
		method = (char *) dbi_result_get_string(dbr, "method");
		start  = (char *) dbi_result_get_string(dbr, "start");
	}
	
	return init_exec(method);
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
		return XMLRPC_UtilityCreateFault(403, "Forbidden");
	
	name = XMLRPC_VectorGetStringWithID(params, "name");
	
	if (!name)
		return XMLRPC_UtilityCreateFault(400, "Bad Request");
	
	if (vxdb_getxid(name, &xid) == -1)
		return XMLRPC_UtilityCreateFault(404, "Not Found");
	
	/* start here */
	if (setup_context() == -1 ||
	    setup_network() == -1 ||
	    setup_namespace() == -1 ||
	    setup_disklimit() == -1 ||
	    guest_init() == -1) {
		cleanup_on_exit();
		return XMLRPC_UtilityCreateFault(500, errmsg);
	}
	
	cleanup_on_exit();
	return params;
}
