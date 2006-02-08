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
#include <arpa/inet.h>
#include <vserver.h>

#include "vc.h"
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
	vc_printf("Usage:\n\n"
	          "ix -get-attr <file>\n"
	          "   -set-attr <file> <list>\n"
	          "   -get-xid  <file>\n"
	          "   -set-xid  <file> <xid>\n");
	exit(rc);
}

int main(int argc, char *argv[])
{
	VC_INIT_ARGV0
	
	int c;
	char *file, *buf;
	
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
		vc_errp("vx_get_iattr");
	
	if (vc_list32_tostr(vc_iattr_list, iattr.flags, &buf, '\n') == -1)
		vc_errp("vc_list32_tostr");
	
	vc_printf("%s\n", buf);
	
	goto out;
	
setattr:
	if (argc > optind) {
		if (vc_list32_parse(argv[optind], vc_iattr_list, &iattr.flags, &iattr.mask, '~', ',') == -1)
			vc_errp("vc_list32_parse");
		
		if (vx_set_iattr(&iattr) == -1)
			vc_errp("vx_set_iattr");
		
		goto out;
	}
	
	else
		goto usage;
	
setxid:
	if (argc > optind) {
		iattr.xid = atoi(argv[optind]);
		
		iattr.flags |= IATTR_XID;
		iattr.mask  |= IATTR_XID;
		
		if (vx_set_iattr(&iattr) == -1)
			vc_errp("vx_set_iattr");
		
		goto out;
	}
	
	else
		goto usage;
	
getxid:
	if (vx_get_iattr(&iattr) == -1)
		vc_errp("vx_get_iattr");
	
	if (iattr.mask & IATTR_XID)
		vc_printf("%d\n", iattr.xid);
	
	goto out;
	
usage:
	usage(EXIT_FAILURE);

out:
	exit(EXIT_SUCCESS);
}
