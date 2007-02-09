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

#define VCD_CAP_AUTH   (1ULL <<  0)
#define VCD_CAP_DLIM   (1ULL <<  1)
#define VCD_CAP_INIT   (1ULL <<  2)
#define VCD_CAP_MOUNT  (1ULL <<  3)
#define VCD_CAP_NET    (1ULL <<  4)
#define VCD_CAP_BCAP   (1ULL <<  5)
#define VCD_CAP_CCAP   (1ULL <<  6)
#define VCD_CAP_CFLAG  (1ULL <<  7)
#define VCD_CAP_RLIM   (1ULL <<  8)
#define VCD_CAP_SCHED  (1ULL <<  9)
#define VCD_CAP_UNAME  (1ULL << 10)
#define VCD_CAP_CREATE (1ULL << 11)
#define VCD_CAP_EXEC   (1ULL << 12)
#define VCD_CAP_INFO   (1ULL << 13)
#define VCD_CAP_HELPER (1ULL << 63)

int auth_isvalid(const char *user, const char *pass);
int auth_isadmin(const char *user);
int auth_capable(const char *user, uint64_t caps);
int auth_isowner(const char *user, const char *name);
int auth_getuid(const char *user);
int auth_getnextuid(void);

#endif
