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
#include <sys/ioctl.h>
#include <vserver.h>

#include "tools.h"

#ifndef VROOT_SET_DEV
#define VROOT_SET_DEV 0x5600
#endif

#ifndef VROOT_CLR_DEV
#define VROOT_CLR_DEV 0x5601
#endif

static const char *rcsid = "$Id$";

static
struct option long_opts[] = {
	COMMON_LONG_OPTS
	{ "set-dev",   1, 0, 0x10 },
	{ "clear-dev", 1, 0, 0x11 },
	{ NULL,        0, 0, 0 },
};

static inline
void usage(int rc)
{
	printf("Usage:\n\n"
	          "vr -set-dev   <dev> <realdev>\n"
	          "   -clear-dev <dev>\n");
	exit(rc);
}

int main(int argc, char *argv[])
{
	INIT_ARGV0
	
	int c, fd;
	char *dev;
	
	/* syscall data */
#define CASE_GOTO(ID, P) case ID: dev = optarg; goto P; break
	
	/* parse command line */
	while (GETOPT(c)) {
		switch (c) {
			COMMON_GETOPT_CASES
			
			CASE_GOTO(0x10, setdev);
			CASE_GOTO(0x11, cleardev);
			
			DEFAULT_GETOPT_CASES
		}
	}
	
#undef CASE_GOTO
	
	goto usage;
	
setdev:
	if (argc <= optind)
		goto usage;
	
	fd = open_read(dev);
	
	if (fd == -1)
		perr("open_read");
	
	int realfd = open_read(argv[optind]);
	
	if (realfd == -1)
		perr("open_read");
	
	if (ioctl(fd, VROOT_SET_DEV, (void *)realfd) == -1)
		perr("ioctl");
	
	close(realfd);
	close(fd);
	
	goto out;
	
cleardev:
	fd = open_read(dev);
	
	if (fd == -1)
		perr("open_read");
	
	if (ioctl(fd, VROOT_CLR_DEV, 0) == -1)
		perr("ioctl");
	
	close(fd);
	
	goto out;
	
usage:
	usage(EXIT_FAILURE);

out:
	exit(EXIT_SUCCESS);
}
