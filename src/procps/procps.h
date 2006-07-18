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

#ifndef _PROCPS_H
#define _PROCPS_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <vserver.h>

#include "lucid.h"

static inline
void die(const char *msg)
{
	dprintf(STDERR_FILENO, msg);
	dprintf(STDERR_FILENO, "\n");
	exit(EXIT_FAILURE);
}

static inline
void pdie(const char *msg)
{
	dprintf(STDERR_FILENO, msg);
	dprintf(STDERR_FILENO, ": ");
	dprintf(STDERR_FILENO, strerror(errno));
	dprintf(STDERR_FILENO, "\n");
	exit(EXIT_FAILURE);
}

static inline
int procps_default_wrapper(int argc, char **argv, char *proc)
{
	xid_t xid;
	
	/* check for xid and shuffle arguments */
	if (argc > 2 && strcmp(argv[1], "--xid") == 0) {
		xid = atoi(argv[2]);
		argv[2] = proc;
		argv = &argv[2];
		argc -= 2;
	}
	
	else {
		xid = 1;
		argv[0] = proc;
	}
	
	/* TODO: change namespace + chroot */
	if (vx_migrate(xid, NULL) == -1)
		pdie("vx_migrate");
	
	if (execvp(argv[0], argv) == -1)
		pdie("execvp");
	
	return EXIT_FAILURE;
}

#endif
