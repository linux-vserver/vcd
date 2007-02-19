// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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

#ifndef _VCD_VALIDATE_H
#define _VCD_VALIDATE_H

#include <stdint.h>
#include <vserver.h>

int validate_name(const char *name);
int validate_xid(xid_t xid);

int validate_path(const char *path);

int validate_dlimits(uint32_t inodes, uint32_t space, int reserved);

int validate_addr(const char *addr);

int validate_username(const char *username);
int validate_password(const char *password);

int validate_groupname(const char *groupname);

int validate_vcd_cap(const char *bcap);

int validate_bcap(const char *bcap);
int validate_ccap(const char *ccap);
int validate_cflag(const char *cflag);
int validate_pflag(const char *pflag);

int validate_rlimit(const char *rlimit);
int validate_rlimits(uint64_t soft, uint64_t max);

int validate_cpuid(int cpuid);
int validate_token_bucket(int fillrate, int interval,
                          int fillrate2, int interval2,
                          int tokensmin, int tokensmax);

int validate_uname(const char *uname);
int validate_uname_value(const char *value);

#endif
