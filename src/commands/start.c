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

#include <stdlib.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <errno.h>
#include <wait.h>
#include <ctype.h>
#include <vserver.h>
#include <limits.h>
#include <lucid/argv.h>
#include <lucid/mmap.h>
#include <lucid/open.h>
#include <lucid/sys.h>

#include "vc.h"

#include "commands.h"
#include "pathconfig.h"

#ifndef MS_REC
#define MS_REC 16384
#endif

#ifndef CLONE_NEWNS
#define CLONE_NEWNS 0x00020000
#endif

static char *start_rcsid = "$Id$";
static xid_t start_xid   = 0;
static char *start_name  = NULL;
static char *start_vdir  = NULL;

void start_help(int rc)
{
	vc_printf("start: Start a virtual server.\n"
	          "usage: start <name>\n"
	          "\n"
	          "%s\n", start_rcsid);
	
	exit(rc);
}

void start_setup_context(void)
{
	int i;
	
	char *buf, *p;
	uint32_t buf32;
	uint64_t buf64;
	
	/* 0) setup syscall data */
	struct vx_create_flags create_flags = {
		.flags = VXF_PERSISTANT,
	};
	
	struct vx_caps caps = {
		.bcaps = ~(VC_INSECURE_BCAPS),
		.ccaps = VC_SECURE_CCAPS,
		.cmask = VC_SECURE_CCAPS,
	};
	
	struct vx_flags cflags = {
		.flags = VC_SECURE_CFLAGS,
		.mask  = VC_SECURE_CFLAGS,
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
	
	struct vx_vhi_name vhiname = {
		.field = 0,
		.name  = "",
	};
	
	/* 1) create context */
	int status;
	pid_t pid = fork();
	
	switch (pid) {
		case -1:
			vc_errp("fork");
		
		case 0:
			if (vx_create(start_xid, &create_flags) == -1)
				vc_errp("vx_create(%d)", start_xid);
			
			exit(EXIT_SUCCESS);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				vc_errp("waitpid1");
			
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				vc_err("start_setup_context failed");
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
	}
	
	/* 2a) custom bcaps */
	uint64_t mask; // dummy
	
	if (vc_cfg_get_list(start_name, "vx.bcaps", &buf) != -1)
		if (flist64_parse(buf, vc_bcaps_list, &caps.bcaps, &mask, '~', '\n') == -1)
			vc_errp("flist64_parse(bcaps)");
	
	/* 2b) custom ccaps */
	if (vc_cfg_get_list(start_name, "vx.ccaps", &buf) != -1)
		if (flist64_parse(buf, vc_ccaps_list, &caps.ccaps, &caps.cmask, '~', '\n') == -1)
			vc_errp("flist64_parse(ccaps)");
	
	if (vx_set_caps(start_xid, &caps) == -1)
		vc_errp("vx_set_caps");
	
	/* 3) custom cflags */
	if (vc_cfg_get_list(start_name, "vx.flags", &buf) != -1)
		if (flist64_parse(buf, vc_cflags_list, &cflags.flags, &cflags.mask, '~', '\n') == -1)
			vc_errp("flist64_parse(cflags)");
	
	if (vx_set_flags(start_xid, &cflags) == -1)
		vc_errp("vx_set_flags");
	
	/* 4) context rlimits */
	if (vx_get_rlimit_mask(&rlimit_mask) == -1)
		vc_errp("vx_get_rlimit_mask");
	
	for (i = 0; vc_cfg_map_local[i].key; i++) {
		if (strncmp(vc_cfg_map_local[i].key, "limit.", 6) != 0)
			continue;
		
		buf = 1 + strchr(vc_cfg_map_local[i].key, '.');
		
		if (flist32_getval(vc_rlimit_list, buf, &buf32)) {
			vc_warn("cannot find ID for limit '%s'", buf);
			continue;
		}
		
		if (vc_cfg_get_list(start_name, vc_cfg_map_local[i].key, &buf) == -1)
			continue;
		
		if ((rlimit_mask.minimum   & buf32) != buf32 &&
		    (rlimit_mask.softlimit & buf32) != buf32 &&
		    (rlimit_mask.maximum   & buf32) != buf32) {
			vc_warn("kernel does not support limit %d\n", buf32);
			continue;
		}
		
		rlimit.id = flist32_mask2val(buf32);
		
		if ((p = strsep(&buf, "\n")) == NULL)
			vc_err("invalid configuration for limit '%s'", vc_cfg_map_local[i].key);
		else
			rlimit.minimum = vc_str_to_rlim(p);
		
		if ((p = strsep(&buf, "\n")) == NULL)
			vc_err("invalid configuration for limit '%s'", vc_cfg_map_local[i].key);
		else
			rlimit.softlimit = vc_str_to_rlim(p);
		
		if ((p = strsep(&buf, "\n")) == NULL)
			vc_err("invalid configuration for limit '%s'", vc_cfg_map_local[i].key);
		else
			rlimit.maximum = vc_str_to_rlim(p);
		
		if (vx_set_rlimit(start_xid, &rlimit) == -1)
			vc_errp("vc_set_rlimit(%d)", rlimit.id);
	}
	
	/* 5) context scheduler */
	
	/* 6) context vhi names */
	for (i = 0; vc_cfg_map_local[i].key; i++) {
		if (strncmp(vc_cfg_map_local[i].key, "vhi.", 4) != 0)
			continue;
		
		buf = 1 + strchr(vc_cfg_map_local[i].key, '.');
		
		if (flist32_getval(vc_vhiname_list, buf, &buf32)) {
			vc_warn("cannot find ID for vhi name '%s'", buf);
			continue;
		}
		
		vhiname.field = flist32_mask2val(buf32);
		
		if (vc_cfg_get_list(start_name, vc_cfg_map_local[i].key, &buf) == -1)
			continue;
		
		strncpy(vhiname.name, buf, VHILEN-1);
		vhiname.name[VHILEN-1] = '\0';
		
		if (vx_set_vhi_name(start_xid, &vhiname) == -1)
			vc_errp("vx_set_vhi_name(%d)", vhiname.field);
	}
	
	vhiname.field = VHIN_CONTEXT;
	
	strncpy(vhiname.name, start_name, VHILEN-1);
	vhiname.name[VHILEN-1] = '\0';
	
	if (vx_set_vhi_name(start_xid, &vhiname) == -1)
		vc_errp("vx_set_vhi_name(%d)", vhiname.field);
}

void start_setup_network(void)
{
	char *buf, *p;
	
	/* 0) setup syscall data */
	struct nx_create_flags create_flags = {
		.flags = NXF_PERSISTANT,
	};
	
	struct nx_flags nflags = {
		.flags = VC_SECURE_NFLAGS,
		.mask  = VC_SECURE_NFLAGS,
	};
	
	struct nx_addr addr;
	
	/* 1) create network context */
	int status;
	pid_t pid = fork();
	
	switch (pid) {
		case -1:
			vc_errp("fork");
		
		case 0:
			if (nx_create(start_xid, &create_flags) == -1)
				vc_errp("nx_create(%s)", start_name);
			
			exit(EXIT_SUCCESS);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				vc_errp("waitpid2");
			
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				vc_err("start_setup_context failed");
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
	}
	
	/* 2) network context flags */
	if (vc_cfg_get_list(start_name, "nx.flags", &buf) != -1)
		if (flist64_parse(buf, vc_nflags_list, &nflags.flags, &nflags.mask, '~', '\n') == -1)
			vc_errp("flist64_parse(nflags)");
	
	if (nx_set_flags(start_xid, &nflags) == -1)
		vc_errp("nx_set_flags");
	
	/* 3) network context addrs */
	if (vc_cfg_get_list(start_name, "nx.addr", &buf) != -1) {
		while ((p = strsep(&buf, "\n")) != NULL) {
			addr.type  = NXA_TYPE_IPV4;
			addr.count = 1;
			
			if (vc_str_to_addr(p, &addr.ip[0], &addr.mask[0]) == -1)
				vc_warnp("vc_str_to_addr");
			
			if (nx_add_addr(start_xid, &addr) == -1)
				vc_warnp("nx_add_addr");
		}
	}
}

void start_setup_namespace(void)
{
	char *buf, *p;
	
	/* 1) init environment for secure_chdir */
	int rootfd;
	int vdirfd;
	
	if ((rootfd = open("/",  O_RDONLY|O_DIRECTORY)) == -1)
		vc_errp("open(rootfd)");
	
	if ((vdirfd = open(start_vdir, O_RDONLY|O_DIRECTORY)) == -1)
		vc_errp("open(vdirfd)");
	
	/* 2) setup initial mtab */
	if (vc_cfg_get_str(start_name, "ns.mtab", &buf) == -1)
		if (vc_cfg_get_str(NULL, "ns.mtab", &buf) == -1)
			vc_err("default mtab could not be found");
	
	if (vc_secure_chdir(rootfd, vdirfd, "/etc") == -1)
		vc_errp("vc_secure_chdir");
	
	int mtabfd;
	
	if ((mtabfd = open_trunc("mtab")) == -1)
		vc_errp("open_trunc(mtab)");
	
	if (write(mtabfd, buf, strlen(buf)) == -1)
		vc_errp("write(mtab)");
	
	/* 3) mount fstab entries */
	if (vc_cfg_get_str(start_name, "ns.fstab", &buf) == -1)
		if (vc_cfg_get_str(NULL, "ns.fstab", &buf) == -1)
			vc_err("default fstab could not be found");
	
	while ((p = strsep(&buf, "\n")) != 0) {
		while (isspace(*p)) ++p;
		
		if (*p == '#' || *p == '\0')
			continue;
		
		char *src, *dst, *type, *data;
		
		if (vc_str_to_fstab(p, &src, &dst, &type, &data) == -1)
			vc_errp("vc_str_to_fstab");
		
		if (vc_secure_chdir(rootfd, vdirfd, dst) == -1)
			vc_errp("vc_secure_chdir");
		
		pid_t pid;
		int status;
		
		switch((pid = fork())) {
			case -1:
				vc_errp("fork");
			
			case 0:
				if (execl(__MOUNT, __MOUNT, "-n", "-t", type, "-o", data, src, ".", NULL) == -1)
					exit(errno);
				
				exit(EXIT_SUCCESS);
			
			default:
				if (waitpid(pid, &status, 0) == -1)
					vc_errp("waitpid3");
				
				if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
					vc_errp("mount");
				
				if (WIFSIGNALED(status))
					kill(getpid(), WTERMSIG(status));
		}
		
		vc_dprintf(mtabfd, "%s %s %s %s 0 0\n", src, dst, type, data);
	}
	
	/* 4) remount vdir at root */
	if (vc_secure_chdir(rootfd, vdirfd, "/") == -1)
		vc_errp("vc_secure_chdir");
	
	if (mount(".", "/", NULL, MS_BIND|MS_REC, NULL) == -1)
		vc_errp("mount");
	
	/* 5) set namespace */
	if (vx_set_namespace(start_xid) == -1)
		vc_errp("vx_set_namespace");
	
	/* 6) cleanup */
	close(mtabfd);
	close(vdirfd);
	close(rootfd);
}

void start_guest_init(void)
{
	int waitchild = 1;
	char *init;
	
	struct vx_migrate_flags migrate_flags = {
		.flags = 0,
	};
	
	struct vx_flags cflags = {
		.flags = 0,
		.mask  = 0,
	};
	
	int ac;
	char **av;
	
	char *buf;
	
	/* 0) load configuration */
	if (vc_cfg_get_str(start_name, "init.style", &init) == -1)
		vc_errp("vc_cfg_get_str(init.style)");
	
	/* 0a) SysV init */
	if (strcmp(init, "sysvinit") == 0 ||
	    strcmp(init, "init")     == 0 ||
	    strcmp(init, "plain")    == 0) {
		cflags.flags |= VXF_INFO_INIT;
		cflags.mask  |= VXF_INFO_INIT;
		
		if (vx_set_flags(start_xid, &cflags) == -1)
			vc_errp("vx_set_flags");
		
		if (argv_parse("/sbin/init", &ac, &av) == -1)
			vc_errp("argv_parse");
		
		migrate_flags.flags = VXM_SET_INIT|VXM_SET_REAPER;
		waitchild = 0;
	}
	
	/* 0b) rescue shell */
	else if (strcmp(init, "rescue") == 0) {
		if (vc_cfg_get_str(start_name, "rescue.shell", &buf) == -1)
			buf = "/bin/sh";
	
		if (argv_parse(buf, &ac, &av) == -1)
			vc_errp("argv_parse");
	}
	
	/* 0c) invalid init style */
	else
		vc_err("unknown init style: %s", init);
	
	pid_t pid;
	int status;
	
	switch ((pid = fork())) {
		case -1:
			vc_errp("fork");
		
		case 0:
			if (waitchild == 0) {
				close(STDIN_FILENO);
				close(STDOUT_FILENO);
				close(STDERR_FILENO);
			}
			
			/* 1) chroot to vdir */
			if (vx_enter_namespace(start_xid) == -1)
				vc_errp("vx_enter_namespace");
			
			if (chroot(start_vdir) == -1)
				vc_errp("chroot");
			
			/* 2) migrate to context */
			if (nx_migrate(start_xid) == -1)
				vc_errp("nx_migrate");
			
			if (vx_migrate(start_xid, &migrate_flags) == -1)
				vc_errp("vx_migrate");
			
			if (execv(av[0], av) == -1)
				vc_errp("execv");
		
		default:
			/* to wait or not to wait!? */
			switch (waitpid(pid, &status, waitchild == 0 ? WNOHANG : 0)) {
				case -1:
					vc_errp("waitpid4");
				
				case 0:
					break;
				
				default:
					if (WIFEXITED(status))
						exit(WEXITSTATUS(status));
					
					if (WIFSIGNALED(status))
						kill(getpid(), WTERMSIG(status));
			}
	}
	
	/* we should not get here */
	exit(EXIT_FAILURE);
}

void start_sighandler(int sig)
{
	switch (sig) {
		case SIGSEGV:
			SIGSEGV_MSG(start);
			break;
	}
}

void start_main(int argc, char *argv[])
{
	if (argc < 2)
		start_help(EXIT_FAILURE);
	
	start_name = argv[1];
	
	/* 0a) load configuration */
	if (vc_cfg_get_int(start_name, "vx.id", (int *) &start_xid) == -1)
		vc_errp("vc_cfg_get_int(vx.id)");
	
	char *buf;
	char vdir[PATH_MAX];
	
	if (vc_cfg_get_str(start_name, "ns.root", &buf) != -1)
		vc_snprintf(vdir, PATH_MAX, "%s", buf);
	else if (vc_cfg_get_str(NULL, "ns.base", &buf) != -1)
		vc_snprintf(vdir, PATH_MAX, "%s/%s", buf, start_name);
	else
		vc_snprintf(vdir, PATH_MAX, "%s/%s", __VDIRBASE, start_name);
	
	start_vdir = vdir;
	
	/* 0b) setup signal handlers */
	signal(SIGSEGV, start_sighandler);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGINT,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	
	pid_t pid;
	int status;
	
	/* 1) setup context */
	start_setup_context();
	
	/* 2) setup network context */
	start_setup_network();
	
	/* 3) setup namespace */
	pid = sys_clone(CLONE_NEWNS|SIGCHLD, 0);
	
	switch (pid) {
		case -1:
			vc_errp("clone");
		
		case 0:
			start_setup_namespace();
			exit(EXIT_SUCCESS);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				vc_errp("waitpid5");
			
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				vc_err("start_setup_namespace failed");
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
	}
	
	/* 4) guest startup */
	start_guest_init();
	
	struct vx_flags cflags = {
		.flags = 0,
		.mask  = VXF_PERSISTANT,
	};
	
	struct nx_flags nflags = {
		.flags = 0,
		.mask  = NXF_PERSISTANT,
	};
	
	vx_set_flags(start_xid, &cflags);
	nx_set_flags(start_xid, &nflags);
	
	exit(EXIT_SUCCESS);
}
