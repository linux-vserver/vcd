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
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <vserver.h>

#include "vc.h"
#include "tools.h"

static const char *rcsid = "$Id$";

static
struct option long_opts[] = {
	COMMON_LONG_OPTS
	{ "add-path",  1, 0, 0x10 },
	{ "rem-path",  1, 0, 0x11 },
	{ "set-limit", 1, 0, 0x12 },
	{ "get-limit", 1, 0, 0x13 },
	{ NULL,        0, 0, 0 },
};

static inline
void usage(int rc)
{
	vc_printf("Usage:\n\n"
	          "dx -add-path  <xid> <path>\n"
	          "   -rem-path  <xid> <path>\n"
	          "   -set-limit <xid> <path> <su>,<st>,<iu>,<it>,<rr>\n"
	          "   -get-limit <xid> <path>\n");
	exit(rc);
}

int main(int argc, char *argv[])
{
	VC_INIT_ARGV0
	
	int c, i;
	xid_t xid = 0;
	char *buf;
	
	/* syscall data */
	struct vx_dlimit_base base = {
		.filename = NULL,
		.flags    = 0,
	};
	
	struct vx_dlimit dlimit = {
		.filename     = NULL,
		.space_used   = CDLIM_KEEP,
		.space_total  = CDLIM_KEEP,
		.inodes_used  = CDLIM_KEEP,
		.inodes_total = CDLIM_KEEP,
		.reserved     = CDLIM_KEEP,
		.flags        = 0,
	};
	
#define CASE_GOTO(ID, P) case ID: xid = atoi(optarg); goto P; break
	
	/* parse command line */
	while (GETOPT(c)) {
		switch (c) {
			COMMON_GETOPT_CASES
			
			CASE_GOTO(0x10, addpath);
			CASE_GOTO(0x11, rempath);
			CASE_GOTO(0x12, setlimit);
			CASE_GOTO(0x13, getlimit);
			
			DEFAULT_GETOPT_CASES
		}
	}
	
#undef CASE_GOTO
	
	goto usage;
	
addpath:
	if (argc > optind) {
		base.filename = argv[optind];
		
		if (vx_add_dlimit(xid, &base) == -1)
			vc_errp("vx_add_dlimit");
		
		goto out;
	}
	
	else
		goto usage;
	
rempath:
	if (argc > optind) {
		base.filename = argv[optind];
		
		if (vx_rem_dlimit(xid, &base) == -1)
			vc_errp("vx_rem_dlimit");
		
		goto out;
	}
	
	else
		goto usage;
	
setlimit:
	if (argc > optind+1) {
		dlimit.filename = argv[optind];
		
		i = 0;
		
		for (buf = strtok(argv[optind+1], ","); buf != NULL; i++) {
			if (strlen(buf) < 1)
				continue;
			
#define BUF2LIM(BUF) strcasecmp(BUF, "inf") == 0 ? CDLIM_INFINITY : (uint32_t) atoi(BUF)
			
			switch (i) {
				case 0: dlimit.space_used   = BUF2LIM(buf); break;
				case 1: dlimit.space_total  = BUF2LIM(buf); break;
				case 2: dlimit.inodes_used  = BUF2LIM(buf); break;
				case 3: dlimit.inodes_total = BUF2LIM(buf); break;
				case 4: dlimit.reserved     = BUF2LIM(buf); break;
			}
			
#undef BUF2LIM
			
			buf = strtok(NULL, ",");
		}
		
		if (i != 5)
			goto usage;
		
		if (vx_set_dlimit(xid, &dlimit) == -1)
			vc_errp("vx_set_dlimit");
		
		goto out;
	}
	
	else
		goto usage;
	
getlimit:
	if (argc > optind) {
		dlimit.filename = argv[optind];
		
		if (vx_get_dlimit(xid, &dlimit) == -1)
			vc_errp("vx_get_dlimit");
		
#define LIM2OUT(LIM, DELIM) \
	if (LIM == CDLIM_INFINITY) vc_printf("%s%c", "inf", DELIM); \
	else vc_printf("%d%c", LIM, DELIM);
		
		LIM2OUT(dlimit.space_used,   ',');
		LIM2OUT(dlimit.space_total,  ',');
		LIM2OUT(dlimit.inodes_used,  ',');
		LIM2OUT(dlimit.inodes_total, ',');
		LIM2OUT(dlimit.reserved,     '\n');
		
#undef LIM2OUT
		
		goto out;
	}
	
	else
		goto usage;
	
usage:
	usage(EXIT_FAILURE);

out:
	exit(EXIT_SUCCESS);
}
