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
#include <inttypes.h>
#include <string.h>
#include <vserver.h>

#include "tools.h"

static const char *rcsid = "$Id$";

static
struct option long_opts[] = {
	COMMON_LONG_OPTS
	{ "get-attr", 1, 0, 0x10 },
	{ "set-attr", 1, 0, 0x11 },
	{ "get-xid",  1, 0, 0x12 },
	{ "set-xid",  1, 0, 0x13 },
	{ NULL,       0, 0, 0 },
};

static inline
void usage(int rc)
{
	printf("Usage:\n\n"
	          "ix -get-attr <file>\n"
	          "   -set-attr <file> <list>\n"
	          "   -get-xid  <file>\n"
	          "   -set-xid  <file> <xid>\n");
	exit(rc);
}

int main(int argc, char *argv[])
{
	INIT_ARGV0
	
	int c;
	char *buf;
	
	/* syscall data */
	struct vx_iattr iattr = {
		.filename = NULL,
		.xid      = 0,
		.flags    = 0,
		.mask     = 0,
	};
	
#define CASE_GOTO(ID, P) case ID: iattr.filename = optarg; goto P; break
	
	/* parse command line */
	while (GETOPT(c)) {
		switch (c) {
			COMMON_GETOPT_CASES
			
			CASE_GOTO(0x10, getattr);
			CASE_GOTO(0x11, setattr);
			CASE_GOTO(0x12, getxid);
			CASE_GOTO(0x13, setxid);
			
			DEFAULT_GETOPT_CASES
		}
	}
	
#undef CASE_GOTO
	
	goto usage;
	
getattr:
	if (vx_get_iattr(&iattr) == -1)
		perr("vx_get_iattr");
	
	if (!(buf = flist32_tostr(iattr_list, iattr.flags & iattr.mask, ',')))
		perr("flist32_tostr");
	
	if (strlen(buf) > 0)
		printf("%s\n", buf);
	
	free(buf);
	goto out;
	
setattr:
	if (argc <= optind)
		goto usage;
	
	if (flist32_parse(argv[optind], iattr_list, &iattr.flags, &iattr.mask, '~', ',') == -1)
		perr("flist32_parse");
	
	if (vx_set_iattr(&iattr) == -1)
		perr("vx_set_iattr");
	
	goto out;
	
getxid:
	if (vx_get_iattr(&iattr) == -1)
		perr("vx_get_iattr");
	
	if (iattr.mask & IATTR_TAG)
		printf("%" PRIu32 "\n", iattr.xid);
	
	goto out;
	
setxid:
	if (argc <= optind)
		goto usage;
	
	if (vx_get_iattr(&iattr) == -1)
		perr("vx_get_iattr");
	
	if (iattr.mask & IATTR_TAG) {
		iattr.xid = atoi(argv[optind]);
		
		iattr.flags |= IATTR_TAG;
		iattr.mask  |= IATTR_TAG;
		
		if (vx_set_iattr(&iattr) == -1)
			perr("vx_set_iattr");
		
		goto out;
	}
	
	else
		err("%s: IATTR_XID not available", iattr.filename);
	
	goto out;
	
usage:
	usage(EXIT_FAILURE);

out:
	exit(EXIT_SUCCESS);
}
