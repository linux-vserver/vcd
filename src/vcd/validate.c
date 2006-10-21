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
#include <string.h>
#include <arpa/inet.h>
#include <lucid/str.h>

#include "lists.h"
#include <lucid/log.h>
#include "validate.h"

int validate_name(const char *name)
{
	LOG_TRACEME
	
	if (str_isempty(name))
		return 0;
	
	if (strlen(name) < 3 || strlen(name) > 32)
		return 0;
	
	return str_isalnum(name);
}

int validate_xid(xid_t xid)
{
	LOG_TRACEME
	
	return (xid > 1 && xid < 65535);
}

int validate_path(const char *path)
{
	LOG_TRACEME
	
	if (str_isempty(path))
		return 0;
	
	return str_path_isabs(path);
}

int validate_dlimits(uint32_t inodes, uint32_t space, int reserved)
{
	LOG_TRACEME
	
	return (reserved > 0 && reserved < 100);
}

int validate_addr(const char *addr)
{
	LOG_TRACEME
	
	if (str_isempty(addr))
		return 0;
	
	struct in_addr inaddr;
	return inet_pton(AF_INET, addr, &inaddr);
}

int validate_username(const char *username)
{
	LOG_TRACEME
	
	if (str_isempty(username))
		return 0;
	
	return str_isalnum(username);
}

int validate_password(const char *password)
{
	LOG_TRACEME
	
	if (str_isempty(password))
		return 0;
	
	return strlen(password) >= 8;
}

int validate_vcd_cap(const char *cap)
{
	LOG_TRACEME
	
	if (str_isempty(cap))
		return 0;
	
	return flist64_getval(vcd_caps_list, cap) == 0 ? 0 : 1;
}

int validate_bcap(const char *bcap)
{
	LOG_TRACEME
	
	if (str_isempty(bcap))
		return 0;
	
	return flist64_getval(bcaps_list, bcap) == 0 ? 0 : 1;
}

int validate_ccap(const char *ccap)
{
	LOG_TRACEME
	
	if (str_isempty(ccap))
		return 0;
	
	return flist64_getval(ccaps_list, ccap) == 0 ? 0 : 1;
}

int validate_cflag(const char *cflag)
{
	LOG_TRACEME
	
	if (str_isempty(cflag))
		return 0;
	
	return flist64_getval(cflags_list, cflag) == 0 ? 0 : 1;
}

int validate_rlimit(const char *rlimit)
{
	LOG_TRACEME
	
	if (str_isempty(rlimit))
		return 0;
	
	return flist32_getval(rlimit_list, rlimit) == 0 ? 0 : 1;
}

int validate_rlimits(int soft, int max)
{
	LOG_TRACEME
	
	return (soft >= 0 && max >= 0 && max >= soft);
}

int validate_cpuid(int cpuid)
{
	LOG_TRACEME
	
	long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
	
	return (cpuid < ncpu && cpuid >= 0);
}

int validate_token_bucket(int32_t fillrate, int32_t interval,
                          int32_t fillrate2, int32_t interval2,
                          int32_t tokensmin, int32_t tokensmax)
{
	LOG_TRACEME
	
	if (fillrate  < 1 ||
	    interval  < 1 ||
	    tokensmax < 1)
		return 0;
	
	if (fillrate2 < 1 && interval2 > 0)
		return 0;
	
	if (interval2 < 1 && fillrate2 > 0)
		return 0;
	
	if (interval < fillrate || interval2 < fillrate2)
		return 0;
	
	if (tokensmax < fillrate || tokensmax < fillrate2)
		return 0;
	
	if (tokensmin > tokensmax)
		return 0;
	
	return 1;
}

int validate_uname(const char *uname)
{
	LOG_TRACEME
	
	if (str_isempty(uname))
		return 0;
	
	return flist32_getval(uname_list, uname) == 0 ? 0 : 1;
}

int validate_uname_value(const char *value)
{
	LOG_TRACEME
	
	if (str_isempty(value))
		return 0;
	
	return strlen(value) < 65;
}
