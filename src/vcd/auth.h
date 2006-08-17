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

#ifndef _VCD_AUTH_H
#define _VCD_AUTH_H

#include <stdint.h>

#define VCD_CAP_AUTH   (1 <<  0)
#define VCD_CAP_DLIM   (1 <<  1)
#define VCD_CAP_INIT   (1 <<  2)
#define VCD_CAP_MOUNT  (1 <<  3)
#define VCD_CAP_NET    (1 <<  4)
#define VCD_CAP_BCAP   (1 <<  5)
#define VCD_CAP_CCAP   (1 <<  6)
#define VCD_CAP_CFLAG  (1 <<  7)
#define VCD_CAP_RLIM   (1 <<  8)
#define VCD_CAP_SCHED  (1 <<  9)
#define VCD_CAP_UNAME  (1 << 10)
#define VCD_CAP_CREATE (1 << 11)
#define VCD_CAP_EXEC   (1 << 12)
#define VCD_CAP_HELPER (1 << 63)

int auth_isvalid(const char *user, const char *pass);
int auth_isadmin(const char *user);
int auth_capable(const char *user, uint64_t caps);
int auth_isowner(const char *user, const char *name);
int auth_getuid(const char *user);

#endif
