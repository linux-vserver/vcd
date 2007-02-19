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

#include <unistd.h>

#include "lists.h"
#include "validate.h"

#include <lucid/addr.h>
#include <lucid/log.h>
#include <lucid/str.h>

int validate_name(const char *name)
{
	LOG_TRACEME
	return !(str_isempty(name) || str_len(name) < 3 ||
			str_len(name) > 32 || !str_isalnum(name));
}

int validate_xid(xid_t xid)
{
	LOG_TRACEME
	return (xid > 1 && xid < 65535);
}

int validate_path(const char *path)
{
	LOG_TRACEME
	return !(str_isempty(path) || !str_path_isabs(path));
}

int validate_dlimits(uint32_t inodes, uint32_t space, int reserved)
{
	LOG_TRACEME
	return reserved > 0 && reserved < 100 &&
			inodes != CDLIM_KEEP && inodes != CDLIM_INFINITY &&
			space  != CDLIM_KEEP && space  != CDLIM_INFINITY;
}

int validate_addr(const char *addr)
{
	LOG_TRACEME
	return !(str_isempty(addr) || addr_from_str(addr, NULL, NULL) < 1);
}

int validate_username(const char *username)
{
	LOG_TRACEME
	return !(str_isempty(username) || !str_isalnum(username));
}

int validate_password(const char *password)
{
	LOG_TRACEME
	return !(str_isempty(password) || str_len(password) < 8);
}

int validate_groupname(const char *groupname)
{
	LOG_TRACEME
	return !(str_isempty(groupname) || !str_isalnum(groupname));
}

int validate_vcd_cap(const char *cap)
{
	LOG_TRACEME
	return !(str_isempty(cap) || flist64_getval(vcd_caps_list, cap) == 0);
}

int validate_bcap(const char *bcap)
{
	LOG_TRACEME
	return !(str_isempty(bcap) || flist64_getval(bcaps_list, bcap) == 0);
}

int validate_ccap(const char *ccap)
{
	LOG_TRACEME
	return !(str_isempty(ccap) || flist64_getval(ccaps_list, ccap) == 0);
}

int validate_cflag(const char *cflag)
{
	LOG_TRACEME
	return !(str_isempty(cflag) || flist64_getval(cflags_list, cflag) == 0);
}

int validate_rlimit(const char *rlimit)
{
	LOG_TRACEME
	return !(str_isempty(rlimit) || flist32_getval(rlimit_list, rlimit) == 0);
}

int validate_rlimits(uint64_t soft, uint64_t max)
{
	LOG_TRACEME
	return soft != CRLIM_KEEP && soft != CRLIM_INFINITY &&
			max != CRLIM_KEEP && max != CRLIM_INFINITY;
}

int validate_cpuid(int cpuid)
{
	LOG_TRACEME
	return (cpuid >= 0 && cpuid < sysconf(_SC_NPROCESSORS_ONLN));
}

int validate_token_bucket(int fillrate, int interval, int fillrate2,
		int interval2, int tokensmin, int tokensmax)
{
	LOG_TRACEME
	return !(
			(fillrate < 1 || interval < 1 || tokensmax < 1) ||
			(fillrate2 < 1 && interval2 > 0) ||
			(interval2 < 1 && fillrate2 > 0) ||
			(interval < fillrate || interval2 < fillrate2) ||
			(tokensmax < fillrate || tokensmax < fillrate2) ||
			(tokensmin > tokensmax)
	);
}

int validate_uname(const char *uname)
{
	LOG_TRACEME
	return !(str_isempty(uname) || flist32_getval(uname_list, uname) == 0);
}

int validate_uname_value(const char *value)
{
	LOG_TRACEME
	return !(str_isempty(value) || str_len(value) > 64);
}
