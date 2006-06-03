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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <vserver.h>

#include "tools.h"

void do_vlogin(int argc, char *argv[], int ind);

static const char *rcsid = "$Id$";

static
struct option long_opts[] = {
	COMMON_LONG_OPTS
	{ "create",    1, 0, 0x10 },
	{ "migrate",   1, 0, 0x11 },
	{ "set-bcaps", 1, 0, 0x12 },
	{ "set-ccaps", 1, 0, 0x13 },
	{ "set-flags", 1, 0, 0x14 },
	{ "set-limit", 1, 0, 0x15 },
	{ "set-sched", 1, 0, 0x16 },
	{ "set-vhi",   1, 0, 0x17 },
	{ "get-bcaps", 1, 0, 0x18 },
	{ "get-ccaps", 1, 0, 0x19 },
	{ "get-flags", 1, 0, 0x1A },
	{ "get-limit", 1, 0, 0x1B },
	{ "get-vhi",   1, 0, 0x1C },
	{ "kill",      1, 0, 0x1D },
	{ "wait",      1, 0, 0x1E },
	{ "login",     1, 0, 0x1F },
	{ NULL,        0, 0, 0 },
};

static
uint64_t str_to_rlim(char *str)
{
	if (str == NULL)
		return CRLIM_KEEP;
	
	if (strcmp(str, "inf") == 0)
		return CRLIM_INFINITY;
	
	if (strcmp(str, "keep") == 0)
		return CRLIM_KEEP;
	
	return atoi(str);
}

static
char *rlim_to_str(uint64_t lim)
{
	char *buf;
	
	if (lim == CRLIM_INFINITY)
		asprintf(&buf, "%s", "inf");
	
	asprintf(&buf, "%lld", lim);
	
	return buf;
}

static inline
void usage(int rc)
{
	printf("Usage:\n\n"
	          "vx -create    <xid> [<list>] [-- <program> <args>*]\n"
	          "   -migrate   <xid> -- <program> <args>*\n"
	          "   -login     <xid> [-- <program> <args>*]\n"
	          "   -set-bcaps <xid> <list>\n"
	          "   -set-ccaps <xid> <list>\n"
	          "   -set-flags <xid> <list>\n"
	          "   -set-limit <xid> <type>=<min>,<soft>,<hard>*\n"
	          "   -set-sched <xid> <type>=<value>*\n"
	          "   -set-vhi   <xid> <type>=<value>*\n"
	          "   -get-bcaps <xid>\n"
	          "   -get-ccaps <xid>\n"
	          "   -get-flags <xid>\n"
	          "   -get-limit <xid> <type>*\n"
	          "   -get-vhi   <xid> <type>*\n"
	          "   -kill      <xid> [<pid> <sig>]\n"
	          "   -wait      <xid>\n");
	exit(rc);
}

int main(int argc, char *argv[])
{
	INIT_ARGV0
	
	int c, i;
	xid_t xid = 0;
	char *buf;
	uint64_t mask;
	
	/* syscall data */
	struct vx_create_flags cf = {
		.flags = 0,
	};
	
	struct vx_bcaps bcaps = {
		.bcaps = ~(0ULL),
		.bmask = ~(0ULL),
	};
	
	struct vx_ccaps ccaps = {
		.ccaps =   0,
		.cmask =   0,
	};
	
	struct vx_flags flags = {
		.flags = 0,
		.mask  = 0,
	};
	
	struct vx_rlimit rlimit = {
		.id        = 0,
		.minimum   = CRLIM_KEEP,
		.softlimit = CRLIM_KEEP,
		.maximum   = CRLIM_KEEP,
	};
	
	struct vx_sched sched = {
		.set_mask   = 0,
		.fill_rate  = 0,
		.interval   = 0,
		.tokens     = 0,
		.tokens_min = 0,
		.tokens_max = 0,
		.prio_bias  = 0,
	};
	
	struct vx_vhi_name vhiname = {
		.field = 0,
		.name  = "",
	};
	
	struct vx_wait_result wait_result = {
		.reboot_cmd = 0,
		.exit_code  = 0,
	};
	
	struct vx_kill_opts kill_opts = {
		.pid = 0,
		.sig = SIGKILL,
	};
	
#define CASE_GOTO(ID, P) case ID: xid = atoi(optarg); goto P; break
	
	/* parse command line */
	while (GETOPT(c)) {
		switch (c) {
			COMMON_GETOPT_CASES
			
			CASE_GOTO(0x10, create);
			CASE_GOTO(0x11, migrate);
			CASE_GOTO(0x12, setbcaps);
			CASE_GOTO(0x13, setccaps);
			CASE_GOTO(0x14, setflags);
			CASE_GOTO(0x15, setlimit);
			CASE_GOTO(0x16, setsched);
			CASE_GOTO(0x17, setvhi);
			CASE_GOTO(0x18, getbcaps);
			CASE_GOTO(0x19, getccaps);
			CASE_GOTO(0x1A, getflags);
			CASE_GOTO(0x1B, getlimit);
			CASE_GOTO(0x1C, getvhi);
			CASE_GOTO(0x1D, kill);
			CASE_GOTO(0x1E, wait);
			CASE_GOTO(0x1F, login);
			
			DEFAULT_GETOPT_CASES
		}
	}
	
#undef CASE_GOTO
	
	goto usage;
	
create:
	if (argc > optind && strcmp(argv[optind], "--") != 0)
		if (flist64_parse(argv[optind], cflags_list, &cf.flags, &mask, '~', ',') == -1)
			perr("flist64_parse");
	
	if (vx_create(xid, &cf) == -1)
		perr("vx_create");
	
	if (argc > optind+1)
		execvp(argv[optind+1], argv+optind+1);
	
	goto out;

migrate:
	if (vx_migrate(xid, NULL) == -1)
		perr("vx_migrate");
	
	if (argc > optind+1)
		execvp(argv[optind+1], argv+optind+1);
	
	goto out;
	
login:
	if (vx_enter_namespace(xid) == -1)
		perr("vx_enter_namespace");
	
	if (chroot(".") == -1)
		perr("chroot");
	
	if (vx_migrate(xid, NULL) == -1)
		perr("vx_migrate");
	
	do_vlogin(argc, argv, optind);
	
	goto out;
	
setbcaps:
	if (argc <= optind)
		goto usage;
	
	if (flist64_parse(argv[optind], bcaps_list, &bcaps.bcaps, &bcaps.bmask, '~', ',') == -1)
		perr("flist64_parse");
	
	if (vx_set_bcaps(xid, &bcaps) == -1)
		perr("vx_set_bcaps");
	
	goto out;
	
setccaps:
	if (argc <= optind)
		goto usage;
	
	if (flist64_parse(argv[optind], ccaps_list, &ccaps.ccaps, &ccaps.cmask, '~', ',') == -1)
		perr("flist64_parse");
	
	if (vx_set_ccaps(xid, &ccaps) == -1)
		perr("vx_set_ccaps");
	
	goto out;
	
setflags:
	if (argc <= optind)
		goto usage;
	
	if (flist64_parse(argv[optind], cflags_list, &flags.flags, &flags.mask, '~', ',') == -1)
		perr("flist64_parse");
	
	if (vx_set_flags(xid, &flags) == -1)
		perr("vx_set_flags");
	
	goto out;
	
setlimit:
	if (argc <= optind)
		goto usage;
	
	for (i = optind; argc > i; i++) {
		buf = strtok(argv[i], "=");
		
		if (buf == NULL)
			goto usage;
		
		if (!(rlimit.id = flist32_getval(rlimit_list, buf)))
			perr("flist32_getval");
		
		rlimit.id = flist32_mask2val(rlimit.id);
		
		if ((buf = strtok(NULL, ",")) == NULL)
			goto usage;
		else
			rlimit.minimum = str_to_rlim(buf);
		
		if ((buf = strtok(NULL, ",")) == NULL)
			goto usage;
		else
			rlimit.softlimit = str_to_rlim(buf);
		
		if ((buf = strtok(NULL, ",")) == NULL)
			goto usage;
		else
			rlimit.maximum = str_to_rlim(buf);
		
		if (vx_set_rlimit(xid, &rlimit) == -1)
			perr("vx_set_rlimit");
	}
	
	goto out;
	
setsched:
	if (argc <= optind)
		goto usage;
	
	for (i = optind; argc > i; i++) {
		buf = strtok(argv[i], "=");
		
		if (buf == NULL)
			goto usage;
		
		if (!(sched.set_mask = flist32_getval(sched_list, buf)))
			perr("flist32_getval");
		
		buf = strtok(NULL, "=");
		
		if (buf == NULL)
			goto usage;
		
		switch (sched.set_mask) {
			case VXSM_FILL_RATE:
				sched.fill_rate = atoi(buf);
				break;
			
			case VXSM_FILL_RATE2:
				sched.fill_rate = atoi(buf);
				break;
			
			case VXSM_INTERVAL:
				sched.interval = atoi(buf);
				break;
			
			case VXSM_INTERVAL2:
				sched.interval = atoi(buf);
				break;
			
			case VXSM_TOKENS:
				sched.tokens = atoi(buf);
				break;
			
			case VXSM_TOKENS_MIN:
				sched.tokens_min = atoi(buf);
				break;
			
			case VXSM_TOKENS_MAX:
				sched.tokens_max = atoi(buf);
				break;
			
			case VXSM_PRIO_BIAS:
				sched.prio_bias = atoi(buf);
				break;
			
			case VXSM_CPU_ID:
				sched.cpu_id = atoi(buf);
				break;
			
			case VXSM_BUCKET_ID:
				sched.bucket_id = atoi(buf);
				break;
		}
		
		if (vx_set_sched(xid, &sched) == -1)
			perr("vx_set_sched");
	}
	
	goto out;
	
setvhi:
	if (argc <= optind)
		goto usage;
	
	for (i = optind; argc > i; i++) {
		buf = strtok(argv[i], "=");
		
		if (buf == NULL)
			goto usage;
		
		if (!(vhiname.field = flist32_getval(vhiname_list, buf)))
			perr("flist32_getval");
		
		buf = strtok(NULL, "=");
		
		if (buf == NULL)
			goto usage;
		
		strncpy(vhiname.name, buf, VHILEN-1);
		vhiname.name[VHILEN-1] = '\0';
		
		if (vx_set_vhi_name(xid, &vhiname) == -1)
			perr("vx_set_vhi_name");
	}
	
	goto out;
	
getbcaps:
	if (vx_get_bcaps(xid, &bcaps) == -1)
		perr("vx_get_bcaps");
	
	buf = flist64_tostr(bcaps_list, bcaps.bcaps, '\n');
	
	if (!str_isempty(buf))
		printf("%s\n", buf);
	
	free(buf);
	
	goto out;
	
getccaps:
	if (vx_get_ccaps(xid, &ccaps) == -1)
		perr("vx_get_ccaps");
	
	buf = flist64_tostr(ccaps_list, ccaps.ccaps, '\n');
	
	if (!str_isempty(buf))
		printf("%s\n", buf);
	
	free(buf);
	
	goto out;
	
getflags:
	if (vx_get_flags(xid, &flags) == -1)
		perr("vx_get_flags");
	
	buf = flist64_tostr(cflags_list, flags.flags, '\n');
	
	if (!str_isempty(buf))
		printf("%s\n", buf);
	
	free(buf);
	
	goto out;
	
getlimit:
	if (argc <= optind)
		goto usage;
	
	for (i = optind; argc > i; i++) {
		if (!(rlimit.id = flist32_getval(rlimit_list, argv[i])))
			perr("flist32_getval");
		
		if (vx_get_rlimit(xid, &rlimit) == -1)
			perr("vx_get_rlimit");
		
		buf = rlim_to_str(rlimit.minimum);
		printf("%s,", buf);
		free(buf);
		
		buf = rlim_to_str(rlimit.softlimit);
		printf("%s,", buf);
		free(buf);
		
		buf = rlim_to_str(rlimit.maximum);
		printf("%s\n", buf);
		free(buf);
	}
	
	goto out;
	
getvhi:
	if (argc <= optind)
		goto usage;
	
	for (i = optind; argc > i; i++) {
		if (!(vhiname.field = flist32_getval(vhiname_list, argv[i])))
			perr("flist32_getval");
		
		if (vx_get_vhi_name(xid, &vhiname) == -1)
			perr("vx_get_vhi_name");
		
		printf("%s=%s\n", argv[i], vhiname.name);
	}
	
	goto out;
	
wait:
	if (vx_wait(xid, &wait_result) == -1)
		perr("vx_wait");
	
	printf("reboot_cmd: %d\n", wait_result.reboot_cmd);
	printf("exit_code:  %d\n", wait_result.exit_code);
	
	goto out;
	
kill:
	if (argc > optind+1) {
		kill_opts.pid = atoi(argv[optind]);
		kill_opts.sig = atoi(argv[optind+1]);
	}
	
	else if (argc > optind)
		goto usage;
	
	if (vx_kill(xid, &kill_opts) == -1)
		perr("vx_kill");
	
	goto out;
	
usage:
	usage(EXIT_FAILURE);

out:
	exit(EXIT_SUCCESS);
}
