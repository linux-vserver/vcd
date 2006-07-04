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
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "msg.h"

/* save argv[0] for error functions below */
const char *argv0;

void warn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	dprintf(STDERR_FILENO, "%s: ", argv0);
	vdprintf(STDERR_FILENO, fmt, ap);
	dprintf(STDERR_FILENO, "\n");
}

void pwarn(const char *fmt, ...)
{
	char *errstr = strerror(errno);
	va_list ap;
	va_start(ap, fmt);
	
	dprintf(STDERR_FILENO, "%s: ", argv0);
	vdprintf(STDERR_FILENO, fmt, ap);
	dprintf(STDERR_FILENO, ": %s\n", errstr);
}

void err(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	dprintf(STDERR_FILENO, "%s: ", argv0);
	vdprintf(STDERR_FILENO, fmt, ap);
	dprintf(STDERR_FILENO, "\n");
	
	exit(EXIT_FAILURE);
}

void perr(const char *fmt, ...)
{
	char *errstr = strerror(errno);
	va_list ap;
	va_start(ap, fmt);
	
	dprintf(STDERR_FILENO, "%s: ", argv0);
	vdprintf(STDERR_FILENO, fmt, ap);
	dprintf(STDERR_FILENO, ": %s\n", errstr);
	
	exit(EXIT_FAILURE);
}
