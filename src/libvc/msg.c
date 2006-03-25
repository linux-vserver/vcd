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
#include <errno.h>
#include <string.h>

#include "vc.h"

/* save argv[0] for error functions below */
const char *vc_argv0;

/* warnings */
void vc_warn(const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	vc_dprintf(STDERR_FILENO, "%s: ", vc_argv0);
	vc_vdprintf(STDERR_FILENO, fmt, ap);
	vc_dprintf(STDERR_FILENO, "\n");
}

void vc_warnp(const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	vc_dprintf(STDERR_FILENO, "%s: ", vc_argv0);
	vc_vdprintf(STDERR_FILENO, fmt, ap);
	vc_dprintf(STDERR_FILENO, ": %s\n", strerror(errno));
}

void vc_err(const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	vc_dprintf(STDERR_FILENO, "%s: ", vc_argv0);
	vc_vdprintf(STDERR_FILENO, fmt, ap);
	vc_dprintf(STDERR_FILENO, "\n");
	
	exit(EXIT_FAILURE);
}

void vc_errp(const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	vc_dprintf(STDERR_FILENO, "%s: ", vc_argv0);
	vc_vdprintf(STDERR_FILENO, fmt, ap);
	vc_dprintf(STDERR_FILENO, ": %s\n", strerror(errno));
	
	exit(EXIT_FAILURE);
}
