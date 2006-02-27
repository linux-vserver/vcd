/***************************************************************************
 *   Copyright 2005 by the vserver-utils team                              *
 *   See AUTHORS for details                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdlib.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <errno.h>
#include <wait.h>
#include <ctype.h>
#include <vserver.h>
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

void start_usage(int rc)
{
	vc_printf("start: Start a virtual server.\n"
	          "usage: start <name>\n"
	          "\n"
	          "%s\n", start_rcsid);
	
	exit(rc);
}

static
void start_sighandler(int sig)
{
	signal(sig, SIG_DFL);
	
	switch(sig) {
		case SIGABRT:
			exit(EXIT_FAILURE);
		
		default:
			kill(getpid(), sig);
			break;
	}
}

void start_setup_context(void)
{
	int i;
	
	char *buf, *p;
	uint32_t buf32;
	uint64_t buf64;
	
	/* 1) setup syscall data */
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
			vc_abortp("clone");
		
		case 0:
			if (vx_create(start_xid, &create_flags) == -1)
				vc_abortp("vx_create(%d)", start_xid);
			
			_exit(0);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				vc_abortp("waitpid");
			
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				vc_abort("start_setup_context failed");
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
	}
	
	/* 2a) custom bcaps */
	uint64_t mask; // dummy
	
	if (vc_cfg_get_list(start_name, "vx.bcaps", &buf) != -1)
		if (flist64_parse(buf, vc_bcaps_list, &caps.bcaps, &mask, '~', '\n') == -1)
			vc_abortp("flist64_parse(bcaps)");
	
	/* 2b) custom ccaps */
	if (vc_cfg_get_list(start_name, "vx.ccaps", &buf) != -1)
		if (flist64_parse(buf, vc_ccaps_list, &caps.ccaps, &caps.cmask, '~', '\n') == -1)
			vc_abortp("flist64_parse(ccaps)");
	
	if (vx_set_caps(start_xid, &caps) == -1)
		vc_abortp("vx_set_caps");
	
	/* 3) custom cflags */
	if (vc_cfg_get_list(start_name, "vx.flags", &buf) != -1)
		if (flist64_parse(buf, vc_cflags_list, &cflags.flags, &cflags.mask, '~', '\n') == -1)
			vc_abortp("flist64_parse(cflags)");
	
	if (vx_set_flags(start_xid, &cflags) == -1)
		vc_abortp("vx_set_flags");
	
	/* 4) context rlimits */
	if (vx_get_rlimit_mask(&rlimit_mask) == -1)
		vc_abortp("vx_get_rlimit_mask");
	
	for (i = 0; vc_cfg_map[i].key; i++) {
		if (strncmp(vc_cfg_map[i].key, "limit.", 6) != 0)
			continue;
		
		buf = 1 + strchr(vc_cfg_map[i].key, '.');
		
		if (flist32_getval(vc_rlimit_list, buf, &buf32)) {
			vc_warn("cannot find ID for limit '%s'", buf);
			continue;
		}
		
		if (vc_cfg_get_list(start_name, vc_cfg_map[i].key, &buf) == -1)
			continue;
		
		if ((rlimit_mask.minimum   & buf32) != buf32 &&
		    (rlimit_mask.softlimit & buf32) != buf32 &&
		    (rlimit_mask.maximum   & buf32) != buf32) {
			vc_warn("kernel does not support limit %d\n", buf32);
			continue;
		}
		
		rlimit.id = flist32_mask2val(buf32);
		
		if ((p = strsep(&buf, "\n")) == NULL)
			vc_abort("invalid configuration for limit '%s'", vc_cfg_map[i].key);
		else
			rlimit.minimum = vc_str_to_rlim(p);
		
		if ((p = strsep(&buf, "\n")) == NULL)
			vc_abort("invalid configuration for limit '%s'", vc_cfg_map[i].key);
		else
			rlimit.softlimit = vc_str_to_rlim(p);
		
		if ((p = strsep(&buf, "\n")) == NULL)
			vc_abort("invalid configuration for limit '%s'", vc_cfg_map[i].key);
		else
			rlimit.maximum = vc_str_to_rlim(p);
		
		if (vx_set_rlimit(start_xid, &rlimit) == -1)
			vc_abortp("vc_set_rlimit(%d)", rlimit.id);
	}
	
	/* 5) context scheduler */
	
	/* 6) context vhi names */
	for (i = 0; vc_cfg_map[i].key; i++) {
		if (strncmp(vc_cfg_map[i].key, "vhi.", 4) != 0)
			continue;
		
		buf = 1 + strchr(vc_cfg_map[i].key, '.');
		
		if (flist32_getval(vc_vhiname_list, buf, &buf32)) {
			vc_warn("cannot find ID for vhi name '%s'", buf);
			continue;
		}
		
		vhiname.field = flist32_mask2val(buf32);
		
		if (vc_cfg_get_list(start_name, vc_cfg_map[i].key, &buf) == -1)
			continue;
		
		strncpy(vhiname.name, buf, VHILEN-1);
		vhiname.name[VHILEN-1] = '\0';
		
		if (vx_set_vhi_name(start_xid, &vhiname) == -1)
			vc_abortp("vx_set_vhi_name(%d)", vhiname.field);
	}
	
	vhiname.field = VHIN_CONTEXT;
	
	strncpy(vhiname.name, start_name, VHILEN-1);
	vhiname.name[VHILEN-1] = '\0';
	
	if (vx_set_vhi_name(start_xid, &vhiname) == -1)
		vc_abortp("vx_set_vhi_name(%d)", vhiname.field);
}

void start_setup_network(void)
{
	char *buf, *p;
	
	/* 1) setup syscall data */
	struct nx_create_flags create_flags = {
		.flags = NXF_PERSISTANT,
	};
	
	struct nx_flags nflags = {
		.flags = VC_SECURE_NFLAGS,
		.mask  = VC_SECURE_NFLAGS,
	};
	
	struct nx_addr addr;
	
	/* 2) create network context */
	int status;
	pid_t pid = fork();
	
	switch (pid) {
		case -1:
			vc_abortp("clone");
		
		case 0:
			if (nx_create(start_xid, &create_flags) == -1)
				vc_abortp("nx_create(%s)", start_name);
			
			_exit(0);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				vc_abortp("waitpid");
			
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				vc_abort("start_setup_context failed");
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
	}
	
	/* 3) network context flags */
	if (vc_cfg_get_list(start_name, "nx.flags", &buf) != -1)
		if (flist64_parse(buf, vc_nflags_list, &nflags.flags, &nflags.mask, '~', '\n') == -1)
			vc_abortp("flist64_parse(nflags)");
	
	if (nx_set_flags(start_xid, &nflags) == -1)
		vc_abortp("nx_set_flags");
	
	/* 4) network context addrs */
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
		vc_abortp("open(rootfd)");
	
	if (vc_cfg_get_str(start_name, "ns.root", &buf) == -1) {
		vc_asprintf(&buf, "%s/%s", __VDIRBASE, start_name);
		vdirfd = open(buf, O_RDONLY|O_DIRECTORY);
		free(buf);
	}
	
	else
		vdirfd = open(buf, O_RDONLY|O_DIRECTORY);
	
	if (vdirfd == -1)
		vc_abortp("open(vdirfd)");
	
	/* 2) setup initial mtab */
	size_t len;
	
	if (vc_cfg_get_str(start_name, "ns.mtab", &buf) == -1)
		buf = mmap_private(__PKGCONFDIR "/mtab", &len);
	
	if (buf == NULL)
		vc_abort("default mtab could not be found");
	
	if (vc_secure_chdir(rootfd, vdirfd, "/etc") == -1)
		vc_abortp("vc_secure_chdir");
	
	int mtabfd = open_trunc("mtab");
	
	if (mtabfd == -1)
		vc_abortp("open_trunc(mtab)");
	
	if (write(mtabfd, buf, len) == -1)
		vc_abortp("write(mtab)");
	
	/* 3) mount fstab entries */
	if (vc_cfg_get_str(start_name, "ns.fstab", &buf) == -1)
		buf = mmap_private(__PKGCONFDIR "/fstab", &len);
	
	if (buf == NULL)
		vc_abort("default fstab could not be found");
	
	while ((p = strsep(&buf, "\n")) != 0) {
		while (isspace(*p)) ++p;
		
		if (*p == '#' || *p == '\0')
			continue;
		
		char *src, *dst, *type, *data;
		
		if (vc_str_to_fstab(p, &src, &dst, &type, &data) == -1)
			vc_abortp("vc_str_to_fstab");
		
		if (vc_secure_chdir(rootfd, vdirfd, dst) == -1)
			vc_abortp("vc_secure_chdir");
		
		pid_t pid;
		int status;
		
		switch((pid = fork())) {
			case -1:
				vc_abortp("fork");
			
			case 0:
				if (execl(__MOUNT, __MOUNT, "-n", "-t", type, "-o", data, src, ".", NULL) == -1)
					exit(errno);
				
				exit(EXIT_SUCCESS);
			
			default:
				if (waitpid(pid, &status, 0) == -1)
					vc_abortp("waitpid");
				
				if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
					vc_abortp("mount");
				
				if (WIFSIGNALED(status))
					kill(getpid(), WTERMSIG(status));
		}
		
		vc_dprintf(mtabfd, "%s %s %s %s 0 0\n", src, dst, type, data);
	}
	
	close(mtabfd);
	
	/* 4) remount vdir at root */
	if (vc_secure_chdir(rootfd, vdirfd, "/") == -1)
		vc_abortp("vc_secure_chdir");
	
	if (mount(".", "/", NULL, MS_BIND|MS_REC, NULL) == -1)
		vc_abortp("mount");
	
	/* 5) set namespace */
	if (vx_set_namespace(start_xid) == -1)
		vc_abortp("vx_set_namespace");
}

void start_guest_init(void)
{
	/* 1) migrate to context/namespace */
	if (nx_migrate(start_xid) == -1)
		vc_abortp("nx_migrate");
	
	if (vx_enter_namespace(start_xid) == -1)
		vc_abortp("vx_enter_namespace");
	
	if (vx_migrate(start_xid) == -1)
		vc_abortp("vx_migrate");
	
	/* 2) chdir to vdir */
	int vdirfd;
	char *buf;
	
	if (vc_cfg_get_str(start_name, "ns.root", &buf) == -1) {
		vc_asprintf(&buf, "%s/%s", __VDIRBASE, start_name);
		vdirfd = open(buf, O_RDONLY|O_DIRECTORY);
		free(buf);
	}
	
	else
		vdirfd = open(buf, O_RDONLY|O_DIRECTORY);
	
	if (vdirfd == -1)
		vc_abortp("open(vdirfd)");
	
	if (fchdir(vdirfd) == -1)
		vc_abortp("fchdir(vdirfd)");
	
	/* 3) guest init */
	pid_t pid;
	int status;
	
	if (vc_cfg_get_str(start_name, "init.style", &buf) == -1)
		vc_abortp("vc_cfg_get_str(init.style)");
	
	/* 4a) plain SysV init */
	if (strcmp(buf, "sysvinit") == 0 ||
	    strcmp(buf, "init") == 0 ||
	    strcmp(buf, "plain") == 0) {
		signal(SIGCHLD, SIG_IGN);
		
		switch (vfork()) {
			case -1:
				vc_abortp("clone");
			
			case 0:
				if (chroot(".") == -1)
					vc_abortp("chroot");
				
				/* TODO: vx_set_initpid(start_xid, getpid()); */
				
				if (execl("/sbin/init", "/sbin/init", NULL) == -1)
					vc_abortp("execl");
			
			default:
				break;
		}
	}
	
	/* 4b) rescue shell */
	else if (strcmp(buf, "rescue") == 0) {
		int ac;
		char **av;
		
		if (vc_cfg_get_str(start_name, "rescue.shell", &buf) == -1) {
			if (argv_parse("/bin/sh", &ac, &av) == -1)
				vc_abortp("argv_parse");
		}
		
		else {
			if (argv_parse(buf, &ac, &av) == -1)
				vc_abortp("argv_parse");
		}
		
		switch ((pid = fork())) {
			case -1:
				vc_abortp("fork");
			
			case 0:
				if (chroot(".") == -1)
					vc_abortp("chroot");
				
				if (execvp(av[0], av) == -1)
					vc_abortp("execvp");
			
			default:
				if (waitpid(pid, &status, 0) == -1)
					vc_abortp("waitpid");
				
				if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
					abort();
				
				if (WIFSIGNALED(status))
					kill(getpid(), WTERMSIG(status));
		}
	}
	
	/* 4c) invalid init style */
	else
		vc_abort("unknown init style: %s", buf);
	
}

void start_main(int argc, char *argv[])
{
	VC_INIT_ARGV0
	
	if (argc < 2)
		start_usage(EXIT_FAILURE);
	
	start_name = argv[1];
	
	/* 0a) load configuration */
	if (vc_cfg_get_int(start_name, "vx.id", (int *) &start_xid) == -1)
		vc_errp("vc_cfg_get_int(vx.id)");
	
	/* 0b) setup signal handlers */
	signal(SIGHUP,  start_sighandler);
	signal(SIGINT,  start_sighandler);
	signal(SIGQUIT, start_sighandler);
	signal(SIGABRT, start_sighandler);
	signal(SIGSEGV, start_sighandler);
	signal(SIGTERM, start_sighandler);
	
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
			vc_abortp("clone");
		
		case 0:
			start_setup_namespace();
			_exit(0);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				vc_abortp("waitpid");
			
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				vc_abort("start_setup_namespace failed");
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
	}
	
	/* 4) guest startup */
	pid = fork();
	
	switch (pid) {
		case -1:
			vc_abortp("clone");
		
		case 0:
			start_guest_init();
			_exit(0);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				vc_abortp("waitpid");
			
			if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS)
				vc_abort("start_guest_init failed");
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
	}
	
	/* 5) cleanup */
	struct vx_flags cflags = {
		.flags = 0,
		.mask  = VXF_PERSISTANT,
	};
	
	struct nx_flags nflags = {
		.flags = 0,
		.mask  = NXF_PERSISTANT,
	};
	
	if (vx_set_flags(start_xid, &cflags) == -1)
		vc_abortp("vx_set_flags");
	
	if (nx_set_flags(start_xid, &nflags) == -1)
		vc_abortp("nx_set_flags");
	
	exit(EXIT_SUCCESS);
}
