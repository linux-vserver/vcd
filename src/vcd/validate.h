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

#ifndef _VCD_VALIDATE_H
#define _VCD_VALIDATE_H

#include <stdint.h>
#include <vserver.h>

int validate_name(char *name);
int validate_xid(xid_t xid);

int validate_path(char *path);

int validate_dlimits(uint32_t inodes, uint32_t space, int reserved);

int validate_init_method(char *method);
int validate_runlevel(char *runlevel);

int validate_addr(char *addr);

int validate_username(char *username);
int validate_password(char *password);

int validate_bcap(char *bcap);
int validate_ccap(char *ccap);
int validate_cflag(char *cflag);
int validate_pflag(char *pflag);

int validate_rlimit(char *rlimit);
int validate_rlimits(int soft, int max);

int validate_cpuid(int cpuid);
int validate_token_bucket(int32_t fillrate, int32_t interval,
                          int32_t fillrate2, int32_t interval2,
                          int32_t tokensmin, int32_t tokensmax,
                          int32_t priobias);

int validate_uname(char *uname);
int validate_uname_value(char *value);

#endif
