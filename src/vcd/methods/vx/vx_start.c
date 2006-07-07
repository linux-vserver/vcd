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

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <vserver.h>
#include <sys/mount.h>
#include <sys/wait.h>

#include "lucid.h"

#include "auth.h"
#include "cfg.h"
#include "lists.h"
#include "log.h"
#include "methods.h"
#include "validate.h"
#include "vxdb.h"

#ifndef MS_REC
#define MS_REC 16384
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

static const char *name = NULL;
static xid_t xid = 0;

static
xmlrpc_value *context_create(xmlrpc_env *env)
{
	struct vx_create_flags create_flags = {
		.flags = VXF_PERSISTENT,
	};
	
	int status;
	pid_t pid = fork();
	
	switch (pid) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
	
	case 0:
		usleep(200);
		
		if (vx_create(xid, &create_flags) == -1)
			exit(errno);
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			method_return_faultf(env, MESYS, "%s: waitpid: %s", __FUNCTION__, strerror(errno));
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			method_return_faultf(env, MESYS, "vx_create: %s", strerror(WEXITSTATUS(status)));
		
		if (WIFSIGNALED(status))
			method_return_faultf(env, MESYS, "%s: caught signal %d", __FUNCTION__, WTERMSIG(status));
	}
	
	return NULL;
}

static
xmlrpc_value *context_caps_and_flags(xmlrpc_env *env)
{
	dbi_result dbr;
	
	struct vx_bcaps bcaps = {
		.bcaps = 0,
		.bmask = 0,
	};
	
	struct vx_ccaps ccaps = {
		.ccaps = 0,
		.cmask = 0,
	};
	
	struct vx_flags cflags = {
		.flags = 0,
		.mask  = 0,
	};
	
	dbr = dbi_conn_queryf(vxdb, "SELECT bcap FROM vx_bcaps WHERE xid = %d", xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	while (dbi_result_next_row(dbr))
		bcaps.bmask |= flist64_getval(bcaps_list, dbi_result_get_string(dbr, "bcap"));
	
	if (vx_set_bcaps(xid, &bcaps) == -1)
		method_return_faultf(env, MESYS, "vx_set_bcaps: %s", strerror(errno));
	
	dbr = dbi_conn_queryf(vxdb, "SELECT ccap FROM vx_ccaps WHERE xid = %d", xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	while (dbi_result_next_row(dbr))
		ccaps.ccaps |= flist64_getval(ccaps_list, dbi_result_get_string(dbr, "ccap"));
	
	ccaps.cmask = ccaps.ccaps;
	
	if (vx_set_ccaps(xid, &ccaps) == -1)
		method_return_faultf(env, MESYS, "vx_set_ccaps: %s", strerror(errno));
	
	dbr = dbi_conn_queryf(vxdb, "SELECT flag FROM vx_flags WHERE xid = %d", xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	while (dbi_result_next_row(dbr))
		cflags.flags |= flist64_getval(cflags_list, dbi_result_get_string(dbr, "flag"));
	
	cflags.mask = cflags.flags;
	
	if (vx_set_flags(xid, &cflags) == -1)
		method_return_faultf(env, MESYS, "vx_set_flags: %s", strerror(errno));
	
	return NULL;
}

static
xmlrpc_value *context_resource_limits(xmlrpc_env *env)
{
	dbi_result dbr;
	const char *type;
	uint32_t buf32;
	
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
	
	if (vx_get_rlimit_mask(&rlimit_mask) == -1)
		method_return_faultf(env, MESYS, "vx_get_rlimit_mask: %s", strerror(errno));
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT type,soft,max FROM vx_limit WHERE xid = %d",
		xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	while (dbi_result_next_row(dbr)) {
		type = dbi_result_get_string(dbr, "type");
		
		if (!(buf32 = flist32_getval(rlimit_list, type)))
			continue;
		
		if ((rlimit_mask.softlimit & buf32) != buf32 &&
		    (rlimit_mask.maximum   & buf32) != buf32)
			continue;
		
		rlimit.id        = flist32_val2index(buf32);
		rlimit.softlimit = dbi_result_get_longlong(dbr, "soft");
		rlimit.maximum   = dbi_result_get_longlong(dbr, "max");
		
		if (vx_set_rlimit(xid, &rlimit) == -1)
			method_return_faultf(env, MESYS, "vx_set_rlimit: %s", strerror(errno));
	}
	
	return NULL;
}

static
xmlrpc_value *context_scheduler(xmlrpc_env *env)
{
	dbi_result dbr;
	
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
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT * FROM vx_sched WHERE xid = %d",
		xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	while (dbi_result_next_row(dbr)) {
		sched.cpu_id     = dbi_result_get_int(dbr, "cpuid");
		sched.fill_rate  = dbi_result_get_int(dbr, "fillrate");
		sched.interval   = dbi_result_get_int(dbr, "interval");
		sched.prio_bias  = dbi_result_get_int(dbr, "priobias");
		sched.tokens_min = dbi_result_get_int(dbr, "tokensmin");
		sched.tokens_max = dbi_result_get_int(dbr, "tokensmax");
		
		sched.tokens = sched.tokens_max;
		
		sched.set_mask |= VXSM_CPU_ID|VXSM_FILL_RATE|VXSM_INTERVAL|VXSM_TOKENS;
		sched.set_mask |= VXSM_TOKENS_MIN|VXSM_TOKENS_MAX|VXSM_PRIO_BIAS;
		
		if (vx_set_sched(xid, &sched) == -1)
			method_return_faultf(env, MESYS, "vx_set_sched: %s", strerror(errno));
		
		sched.fill_rate  = dbi_result_get_int(dbr, "fillrate2");
		sched.interval   = dbi_result_get_int(dbr, "interval2");
		
		sched.set_mask = 0|VXSM_FILL_RATE2|VXSM_INTERVAL2;
		
		if (sched.fill_rate > 0 && sched.interval > 0)
			if (vx_set_sched(xid, &sched) == -1)
				method_return_faultf(env, MESYS, "vx_set_sched2: %s", strerror(errno));
	}
	
	return NULL;
}

static
xmlrpc_value *context_uname(xmlrpc_env *env)
{
	dbi_result dbr;
	const char *uname;
	uint32_t buf32;
	
	struct vx_vhi_name vhiname = {
		.field = 0,
		.name  = "",
	};
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT uname,value FROM vx_uname WHERE xid = %d",
		xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	while (dbi_result_next_row(dbr)) {
		uname = dbi_result_get_string(dbr, "uname");
		
		if (!(buf32 = flist32_getval(vhiname_list, uname)))
			continue;
		
		vhiname.field = flist32_val2index(buf32);
		
		bzero(vhiname.name, VHILEN);
		strncpy(vhiname.name, dbi_result_get_string(dbr, "value"), VHILEN-1);
		
		if (vx_set_vhi_name(xid, &vhiname) == -1)
			method_return_faultf(env, MESYS, "vx_set_vhi_name: %s", strerror(errno));
	}
	
	if (!str_isempty(name)) {
		vhiname.field = VHIN_CONTEXT;
		
		bzero(vhiname.name, VHILEN);
		strncpy(vhiname.name, name, VHILEN-1);
		
		if (vx_set_vhi_name(xid, &vhiname) == -1)
			method_return_faultf(env, MESYS, "vx_set_vhi_name: %s", strerror(errno));
	}
	
	return NULL;
}

static
xmlrpc_value *network_create(xmlrpc_env *env)
{
	struct nx_create_flags create_flags = {
		.flags = NXF_PERSISTENT,
	};
	
	int status;
	pid_t pid = fork();
	
	switch (pid) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
	
	case 0:
		usleep(200);
		
		if (nx_create(xid, &create_flags) == -1)
			exit(errno);
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			method_return_faultf(env, MESYS, "%s: waitpid: %s", __FUNCTION__, strerror(errno));
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			method_return_faultf(env, MESYS, "nx_create: %s", strerror(WEXITSTATUS(status)));
		
		if (WIFSIGNALED(status))
			method_return_faultf(env, MESYS, "%s: caught signal %d", __FUNCTION__, WTERMSIG(status));
	}
	
	return NULL;
}

static
xmlrpc_value *network_interfaces(xmlrpc_env *env)
{
	dbi_result dbr;
	const char *ip, *netm;
	char buf[32];
	struct nx_addr addr;
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT broadcast FROM nx_broadcast WHERE xid = %d",
		xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	if (dbi_result_get_numrows(dbr) > 0) {
		dbi_result_first_row(dbr);
		
		addr.type    = NXA_TYPE_IPV4 | NXA_MOD_BCAST;
		addr.count   = 1;
		
		ip = dbi_result_get_string(dbr, "broadcast");
		
		if (addr_from_str(ip, &addr.ip[0], &addr.mask[0]) == -1)
			method_return_faultf(env, MECONF, "invalid interface: %s", buf);
		
		addr.mask[0] = 0;
		
		if (nx_add_addr(xid, &addr) == -1)
			method_return_faultf(env, MESYS, "nx_add_addr: %s", strerror(errno));
		
	}
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT addr,netmask FROM nx_addr WHERE xid = %d",
		xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	while (dbi_result_next_row(dbr)) {
		ip   = dbi_result_get_string(dbr, "addr");
		netm = dbi_result_get_string(dbr, "netmask");
		
		bzero(buf, 32);
		snprintf(buf, 31, "%s/%s", ip, netm);
		
		addr.type  = NXA_TYPE_IPV4;
		addr.count = 1;
		
		if (addr_from_str(buf, &addr.ip[0], &addr.mask[0]) == -1)
			method_return_faultf(env, MECONF, "invalid interface: %s", buf);
		
		if (nx_add_addr(xid, &addr) == -1)
			method_return_faultf(env, MESYS, "nx_add_addr : %s", strerror(errno));
	}
	
	return NULL;
}

static
xmlrpc_value *mount_namespace(xmlrpc_env *env)
{
	dbi_result dbr;
	int mtabfd, status;
	const char *src, *dst, *type, *opts;
	
	char *vserverdir = cfg_getstr(cfg, "vserverdir");
	char *vdir = NULL;
	
	asprintf(&vdir, "%s/%s", vserverdir, name);
	
	if (chroot_secure_chdir(vdir, "/etc") == -1)
		log_warn_and_die("chroot_secure_chdir: %s", strerror(errno));
	
	if ((mtabfd = open_trunc("mtab")) == -1)
		log_warn_and_die("open_trunc: %s", strerror(errno));
	
	if (write(mtabfd, "/dev/hdv1 / ufs rw 0 0\n", 23) == -1)
		log_warn_and_die("write: %s", strerror(errno));
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT spec,path,vfstype,mntops FROM mount WHERE xid = %d",
		xid);
	
	if (!dbr)
		log_warn_and_die("error in vxdb");
	
	while (dbi_result_next_row(dbr)) {
		src  = dbi_result_get_string(dbr, "spec");
		dst  = dbi_result_get_string(dbr, "path");
		type = dbi_result_get_string(dbr, "vfstype");
		opts = dbi_result_get_string(dbr, "mntops");
		
		if (str_isempty(type))
			type = "auto";
		
		if (str_isempty(opts))
			opts = "defaults";
		
		if (chroot_secure_chdir(vdir, dst) == -1)
			log_warn_and_die("chroot_secure_chdir: %s", strerror(errno));
		
		status = exec_fork("mount -n -t %s -o %s %s .", type, opts, src);
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			log_warn_and_die("mount failed (%d)", WEXITSTATUS(status));
		
		if (WIFSIGNALED(status))
			log_warn_and_die("mount caught signal (%d)", WTERMSIG(status));
		
		dprintf(mtabfd, "%s %s %s %s 0 0\n", src, dst, type, opts);
	}
	
	if (chroot_secure_chdir(vdir, "/") == -1)
		log_warn_and_die("chroot_secure_chdir: %s", strerror(errno));
	
	free(vdir);
	
	if (mount(".", "/", NULL, MS_BIND|MS_REC, NULL) == -1)
		log_warn_and_die("mount: %s", strerror(errno));
	
	close(mtabfd);
	
	if (vx_set_namespace(xid) == -1)
		log_warn_and_die("vx_set_namespace: %s", strerror(errno));
	
	return NULL;
}

static
xmlrpc_value *namespace_create(xmlrpc_env *env)
{
	int status;
	pid_t pid = vx_clone_namespace();
	
	switch (pid) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
	
	case 0:
		usleep(200);
		
		mount_namespace(env);
		
		if (env->fault_occurred)
			exit(env->fault_code);
		 
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			method_return_faultf(env, MESYS, "%s: waitpid: %s", __FUNCTION__, strerror(errno));
		
		if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
			method_return_faultf(env, MESYS, "mount failed (%d)", WEXITSTATUS(status));
		
		if (WIFSIGNALED(status))
			method_return_faultf(env, MESYS, "%s: caught signal %d", __FUNCTION__, WTERMSIG(status));
	}
	
	return NULL;
}

static
xmlrpc_value *disklimit_calc(xmlrpc_env *env)
{
	return NULL;
}

xmlrpc_value *init_init(xmlrpc_env *env, char *vdir)
{
	pid_t pid;
	int i;
	
	struct vx_flags cflags = {
		.flags = VXF_INFO_INIT,
		.mask  = VXF_INFO_INIT,
	};
	
	struct vx_migrate_flags migrate_flags = {
		.flags = VXM_SET_INIT|VXM_SET_REAPER,
	};
	
	if (vx_set_flags(xid, &cflags) == -1)
		method_return_faultf(env, MESYS, "vx_set_flags: %s", strerror(errno));
	
	switch ((pid = fork())) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
	
	case 0:
		for (i = 0; i < 100; i++)
			close(i);
		
		if (vx_enter_namespace(xid) == -1)
			exit(EXIT_FAILURE);
		
		if (chroot_secure_chdir(vdir, "/") == -1)
			exit(EXIT_FAILURE);
	
		if (chroot(".") == -1)
			exit(EXIT_FAILURE);
		
		if (nx_migrate(xid) == -1 || vx_migrate(xid, &migrate_flags) == -1)
			exit(EXIT_FAILURE);
		
		if (exec_replace("/sbin/init") == -1)
			exit(EXIT_FAILURE);
		
		/* never get here */
		exit(EXIT_SUCCESS);
	
	default:
		signal(SIGCHLD, SIG_IGN);
	}
	
	return NULL;
}

xmlrpc_value *init_initng(xmlrpc_env *env, char *vdir)
{
	pid_t pid;
	int i;
	
	struct vx_flags cflags = {
		.flags = VXF_INFO_INIT,
		.mask  = VXF_INFO_INIT,
	};
	
	struct vx_migrate_flags migrate_flags = {
		.flags = VXM_SET_INIT|VXM_SET_REAPER,
	};
	
	if (vx_set_flags(xid, &cflags) == -1)
		method_return_faultf(env, MESYS, "vx_set_flags: %s", strerror(errno));
	
	switch ((pid = fork())) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
	
	case 0:
		for (i = 0; i < 100; i++)
			close(i);
		
		if (vx_enter_namespace(xid) == -1)
			exit(EXIT_FAILURE);
		
		if (chroot_secure_chdir(vdir, "/") == -1)
			exit(EXIT_FAILURE);
	
		if (chroot(".") == -1)
			exit(EXIT_FAILURE);
		
		if (nx_migrate(xid) == -1 || vx_migrate(xid, &migrate_flags) == -1)
			exit(EXIT_FAILURE);
		
		if (exec_replace("/sbin/initng") == -1)
			exit(EXIT_FAILURE);
		
		exit(EXIT_SUCCESS);
	
	default:
		signal(SIGCHLD, SIG_IGN);
	}
	
	return NULL;
}

xmlrpc_value *init_sysvrc(xmlrpc_env *env, char *vdir)
{
	return NULL;
}

xmlrpc_value *init_gentoo(xmlrpc_env *env, char *vdir, const char *runlevel)
{
	pid_t pid;
	int i, status;
	
	if (str_isempty(runlevel))
		runlevel = "default";
	
	switch ((pid = fork())) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
	
	case 0:
		for (i = 0; i < 100; i++)
			close(i);
		
		if (vx_enter_namespace(xid) == -1)
			exit(EXIT_FAILURE);
		
		if (chroot_secure_chdir(vdir, "/") == -1)
			exit(EXIT_FAILURE);
	
		if (chroot(".") == -1)
			exit(EXIT_FAILURE);
		
		if (nx_migrate(xid) == -1 || vx_migrate(xid, NULL) == -1)
			exit(EXIT_FAILURE);
		
		status = exec_fork("/sbin/rc sysinit");
		
		if ((WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) ||
		    WIFSIGNALED(status))
			exit(EXIT_FAILURE);
		
		status = exec_fork("/sbin/rc boot");
		
		if ((WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) ||
		    WIFSIGNALED(status))
			exit(EXIT_FAILURE);
		
		status = exec_fork("/sbin/rc %s", runlevel);
		
		if ((WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) ||
		    WIFSIGNALED(status))
			exit(EXIT_FAILURE);
		
		exit(EXIT_SUCCESS);
	
	default:
		signal(SIGCHLD, SIG_IGN);
	}
	
	return NULL;
}

static
xmlrpc_value *call_init(xmlrpc_env *env)
{
	dbi_result dbr;
	const char *method, *start;
	pid_t pid;
	int status;
	
	char *vserverdir = cfg_getstr(cfg, "vserverdir");
	char *vdir = NULL;
	
	asprintf(&vdir, "%s/%s", vserverdir, name);
	
	dbr = dbi_conn_queryf(vxdb,
		"SELECT method,start FROM init_method WHERE xid = %d",
		xid);
	
	if (!dbr)
		method_return_fault(env, MEVXDB);
	
	if (dbi_result_get_numrows(dbr) < 1) {
		method = "init";
		start = "";
	}
	
	else {
		dbi_result_first_row(dbr);
		method = dbi_result_get_string(dbr, "method");
		start  = dbi_result_get_string(dbr, "start");
	}
	
	if (!validate_init_method(method))
		method_return_faultf(env, MECONF, "unknown init method: %s", method);
	
	switch ((pid = fork())) {
	case -1:
		method_return_faultf(env, MESYS, "%s: fork: %s", __FUNCTION__, strerror(errno));
	
	case 0:
		usleep(200);
		
		if (strcmp(method, "init") == 0)
			init_init(env, vdir);
		
		else if (strcmp(method, "initng") == 0)
			init_initng(env, vdir);
		
		else if (strcmp(method, "sysvrc") == 0)
			init_sysvrc(env, vdir);
		
		else if (strcmp(method, "gentoo") == 0)
			init_gentoo(env, vdir, start);
		
		exit(EXIT_SUCCESS);
	
	default:
		if (waitpid(pid, &status, 0) == -1)
			method_return_faultf(env, MESYS, "%s: waitpid: %s", __FUNCTION__, strerror(errno));
	}
	
	return NULL;
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
xmlrpc_value *m_vx_start(xmlrpc_env *env, xmlrpc_value *p, void *c)
{
	xmlrpc_value *params;
	
	params = method_init(env, p, VCD_CAP_INIT, 1);
	method_return_if_fault(env);
	
	xmlrpc_decompose_value(env, params,
		"{s:s,*}",
		"name", &name);
	method_return_if_fault(env);
	
	if (!validate_name(name))
		method_return_fault(env, MEINVAL);
	
	if (!(xid = vxdb_getxid(name)))
		method_return_fault(env, MENOVPS);
	
	if (vx_get_info(xid, NULL) != -1)
		method_return_fault(env, MERUNNING);
	
	context_create(env);
	method_cleanup_if_fault(env);
	
	context_caps_and_flags(env);
	method_cleanup_if_fault(env);
	
	context_resource_limits(env);
	method_cleanup_if_fault(env);
	
	context_scheduler(env);
	method_cleanup_if_fault(env);
	
	context_uname(env);
	method_cleanup_if_fault(env);
	
	network_create(env);
	method_cleanup_if_fault(env);
	
	network_interfaces(env);
	method_cleanup_if_fault(env);
	
	namespace_create(env);
	method_cleanup_if_fault(env);
	
	disklimit_calc(env);
	method_cleanup_if_fault(env);
	
	call_init(env);
	
cleanup:
	cleanup_on_exit();
	return xmlrpc_nil_new(env);
}
