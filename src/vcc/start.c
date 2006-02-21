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
#include <lucid/mmap.h>
#include <lucid/open.h>

#include "vc.h"

#include "commands.h"
#include "pathconfig.h"

#ifndef MS_REC
#define MS_REC 16384
#endif

static const char *start_rcsid = "$Id$";

/* global storage for cleanup handler */
static int start_state = 0;
static xid_t start_xid = 0;
static char *start_name = NULL;

/* setup states */
#define S_HAVE_VX 0x00000001
#define S_HAVE_NX 0x00000002
#define S_HAVE_NS 0x00000004

#define S_DONE    0x80000000

void start_usage(int rc)
{
	vc_printf("start: Start a virtual server.\n"
	          "usage: start <name>\n"
	          "\n"
	          "%s\n", start_rcsid);
	
	exit(rc);
}

static
void start_cleanup(void)
{
	if (start_state & S_HAVE_NX)
		vc_nx_release(start_xid);
	
	if (start_state & S_HAVE_VX)
		vc_vx_release(start_xid);
	
	if (start_state & S_DONE)
		return;
}

static
void start_sighandler(int sig)
{
	signal(sig, SIG_DFL);
	start_cleanup();
	
	switch(sig) {
		case SIGABRT:
			exit(EXIT_FAILURE);
		
		default:
			kill(getpid(), sig);
			break;
	}
}

void start_main(int argc, char *argv[])
{
	VC_INIT_ARGV0
	
	if (argc < 2)
		start_usage(EXIT_FAILURE);
	
	char *name = argv[1];
	start_name = name;
	
	int i;
	char *buf, *p;
	uint32_t buf32;
	uint64_t buf64;
	
	pid_t pid;
	int status;
	
	/* 0a) load configuration
	**
	** mandatory keys:
	** - context.id
	** - init.style */
	xid_t xid;
	nid_t nid;
	char *style, *vdir;
	
	if (vc_cfg_get_xid(name, &xid) == -1)
		vc_errp("vc_cfg_get_xid(%s)", name);
	
	start_xid = xid;
	nid = (nid_t) xid;
	
	if (vc_cfg_get_str(name, "init.style", &style) == -1)
		vc_errp("vc_cfg_get_str(init.style)");
	
	/* 0b) init environment for secure_chdir */
	int rootfd;
	int vdirfd;
	
	if ((rootfd = open("/",  O_RDONLY|O_DIRECTORY)) == -1)
		vc_errp("open(rootfd)");
	
	if (vc_cfg_get_str(name, "ns.root", &buf) == -1) {
		vc_asprintf(&buf, "%s/%s", __VDIRBASE, name);
		vdirfd = open(buf, O_RDONLY|O_DIRECTORY);
		free(buf);
		
		if (vdirfd == -1)
			vc_errp("open(vdirfd)");
	}
	
	else {
		vdirfd = open(buf, O_RDONLY|O_DIRECTORY);
		
		if (vdirfd == -1)
			vc_errp("open(vdirfd)");
	}
	
	/* 0b) setup signal handlers */
	signal(SIGHUP,  start_sighandler);
	signal(SIGINT,  start_sighandler);
	signal(SIGQUIT, start_sighandler);
	signal(SIGABRT, start_sighandler);
	signal(SIGSEGV, start_sighandler);
	signal(SIGTERM, start_sighandler);
	
	/* 0c) setup syscall data */
	struct vx_caps caps = {
		.bcaps = ~(VC_INSECURE_BCAPS),
		.ccaps = VC_SECURE_CCAPS,
		.cmask = VC_SECURE_CCAPS,
	};
	
	struct vx_flags cflags = {
		.flags = VC_SECURE_CFLAGS,
		.mask  = VC_SECURE_CFLAGS,
	};
	
	struct vx_rlimit rlimit = {
		.id        = -1,
		.minimum   = 0,
		.softlimit = 0,
		.maximum   = 0,
	};
	
	struct vx_rlimit_mask rlimit_mask = {
		.minimum   = 0,
		.softlimit = 0,
		.maximum   = 0,
	};
	
	struct vx_vhi_name vhiname = {
		.field = 0,
		.name  = "",
	};
	
	struct nx_flags nflags = {
		.flags = VC_SECURE_NFLAGS,
		.mask  = VC_SECURE_NFLAGS,
	};
	
	/* 1) create context */
	if (vc_vx_new(xid) == -1)
		vc_abortp("vc_vx_new(%d)", xid);
	
	start_state |= S_HAVE_VX;
	
	/* 1a) custom bcaps */
	uint64_t mask; // dummy
	
	if (vc_cfg_get_list(name, "vx.bcaps", &buf) != -1)
		if (flist64_parse(buf, vc_bcaps_list, &caps.bcaps, &mask, '~', '\n') == -1)
			vc_abortp("flist64_parse(bcaps)");
	
	/* 1b) custom ccaps */
	if (vc_cfg_get_list(name, "vx.ccaps", &buf) != -1)
		if (flist64_parse(buf, vc_ccaps_list, &caps.ccaps, &caps.cmask, '~', '\n') == -1)
			vc_abortp("flist64_parse(ccaps)");
	
	if (vx_set_caps(xid, &caps) == -1)
		vc_abortp("vx_set_caps");
	
	/* 1c) custom cflags */
	if (vc_cfg_get_list(name, "vx.flags", &buf) != -1)
		if (flist64_parse(buf, vc_cflags_list, &cflags.flags, &cflags.mask, '~', '\n') == -1)
			vc_abortp("flist64_parse(cflags)");
	
	if (vx_set_flags(xid, &cflags) == -1)
		vc_abortp("vx_set_flags");
	
	/* 1d) context rlimits */
	if (vx_get_rlimit_mask(&rlimit_mask) == -1)
		vc_abortp("vx_get_rlimit_mask");
	
	for (i = 0; vc_cfg_map[i].key; i++) {
		if (strncmp(vc_cfg_map[i].key, "limit.", 6) != 0)
			continue;
		
		buf = strchr(vc_cfg_map[i].key, '.');
		
		if (flist32_getval(vc_rlimit_list, ++buf, &buf32)) {
			vc_warn("cannot find ID for limit '%s'", buf);
			continue;
		}
		
		if (vc_cfg_get_list(name, vc_cfg_map[i].key, &buf) == -1)
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
		
		if (vx_set_rlimit(xid, &rlimit) == -1)
			vc_abortp("vc_set_rlimit(%d)", rlimit.id);
	}
	
	/* 1e) context scheduler */
	
	/* 1f) context vhi names */
	for (i = 0; vc_cfg_map[i].key; i++) {
		if (strncmp(vc_cfg_map[i].key, "vhi.", 4) != 0)
			continue;
		
		buf = strchr(vc_cfg_map[i].key, '.');
		
		if (flist32_getval(vc_vhiname_list, ++buf, &buf32)) {
			vc_warn("cannot find ID for vhi name '%s'", buf);
			continue;
		}
		
		vhiname.field = flist32_mask2val(buf32);
		
		if (vc_cfg_get_list(name, vc_cfg_map[i].key, &buf) == -1)
			continue;
		
		strncpy(vhiname.name, buf, VHILEN-1);
		vhiname.name[VHILEN-1] = '\0';
		
		if (vx_set_vhi_name(xid, &vhiname) == -1)
			vc_abortp("vx_set_vhi_name(%d)", vhiname.field);
	}
	
	vhiname.field = VHIN_CONTEXT;
	
	strncpy(vhiname.name, name, VHILEN-1);
	vhiname.name[VHILEN-1] = '\0';
	
	if (vx_set_vhi_name(xid, &vhiname) == -1)
		vc_abortp("vx_set_vhi_name(%d)", vhiname.field);
	
	/* 2) create network context */
	if (vc_nx_new(nid) == -1)
		vc_abortp("vc_nx_create(%s)", name);
	
	start_state |= S_HAVE_NX;
	
	/* 2a) network context flags */
	if (vc_cfg_get_list(name, "nx.flags", &buf) != -1)
		if (flist64_parse(buf, vc_nflags_list, &nflags.flags, &nflags.mask, '~', '\n') == -1)
			vc_abortp("flist64_parse(nflags)");
	
	if (nx_set_flags(nid, &nflags) == -1)
		vc_abortp("nx_set_flags");
	
	/* 2b) network context addrs */
	if (vc_cfg_get_list(name, "nx.addr", &buf) != -1) {
		while ((p = strsep(&buf, "\n")) != NULL) {
			if (vc_nx_add_addr(nid, p) == -1)
				vc_warnp("vc_nx_add_addr(%s)", p);
		}
	}
	
	/* 3) create filesystem namespace */
	if (vc_ns_new(xid) == -1)
		vc_abortp("vc_ns_new(%s)", name);
	
	if (vx_enter_namespace(xid) == -1)
		vc_abortp("vx_enter_namespace(%s)", name);
	
	start_state |= S_HAVE_NS;
	
	/* 3a) setup initial mtab */
	size_t len;
	
	if (vc_cfg_get_str(name, "ns.mtab", &buf) == -1)
		buf = mmap_private(__PKGCONFDIR "/mtab", &len);
	
	if (buf == NULL)
		vc_abort("default mtab could not be found");
	
	if (vc_secure_chdir(rootfd, vdirfd, "/etc") == -1)
		vc_abortp("vc_secure_chdir");
	
	if (execl("/bin/sh", "/bin/sh", (char *) NULL) == -1)
		vc_abortp("execl");
	
	int mtabfd = open_trunc("mtab");
	
	if (mtabfd == -1)
		vc_abortp("open_trunc(mtab)");
	
	if (write(mtabfd, buf, len) == -1)
		vc_abortp("write(mtab)");
	
	/* 3b) mount fstab entries */
	if (vc_cfg_get_str(name, "ns.fstab", &buf) == -1)
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
		
		switch((pid = fork())) {
			case -1:
				vc_abortp("fork");
			
			case 0:
				vc_printf("'%s %s %s %s %s %s %s %s'\n", __MOUNT, "-n", "-t", type, "-o", data, src, ".");
				
				if (execl(__MOUNT, __MOUNT, "-n", "-t", type, "-o", data, src, ".", NULL) == -1)
					exit(errno);
				
				exit(EXIT_SUCCESS);
			
			default:
				pid = vc_waitpid(pid);
				
				if (pid == -1)
					vc_abortp("vc_waitpid");
				
				if (pid == -2)
					vc_abortp("failed to mount '%s'", dst);
		}
		
		vc_dprintf(mtabfd, "%s %s %s %s 0 0\n", src, dst, type, data);
	}
	
	close(mtabfd);
	
	/* 3d) remount vdir at root */
	if (vc_secure_chdir(rootfd, vdirfd, "/") == -1)
		vc_abortp("vc_secure_chdir");
	
	if (mount(".", "/", NULL, MS_BIND|MS_REC, NULL) == -1)
		vc_abortp("mount");
	
	if (chroot(".") == -1)
		vc_abortp("chroot");
	
	/* 4) guest startup */
	switch((pid = fork())) {
		case -1:
			vc_abortp("fork");
		
		case 0:
			if (nx_migrate(nid) == -1) {
				vc_warnp("nx_migrate");
				exit(errno);
			}
			
			if (vx_migrate(xid) == -1) {
				vc_warnp("vx_migrate");
				exit(errno);
			}
			if (execl("/bin/sh", "/bin/sh", (char *) NULL) == -1) {
				vc_warnp("execl");
				exit(errno);
			}
			
			exit(EXIT_SUCCESS);
		
		default:
			pid = vc_waitpid(pid);
			
			if (pid == -1)
				vc_abortp("vc_waitpid");
			
			if (pid == -2)
				vc_abort("failed to start guest init");
	}
	
	/* 5) cleanup phase */
	start_state |= S_DONE;
	
	start_cleanup();
	
	exit(EXIT_SUCCESS);
}
