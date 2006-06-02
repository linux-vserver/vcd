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

#include <string.h>
#include <arpa/inet.h>

#include "lucid.h"

#include "lists.h"
#include "validate.h"

int validate_name(char *name)
{
	if (!name)
		return 0;
	
	return str_isalnum(name);
}

int validate_xid(xid_t xid)
{
	return (xid > 2 && xid < 65535);
}

int validate_path(char *path)
{
	if (!path)
		return 0;
	
	return str_path_isabs(path);
}

int validate_dlimits(uint32_t inodes, uint32_t space, int reserved)
{
	return (reserved > 0 && reserved < 100);
}

int validate_init_method(char *method)
{
	if (!method)
		return 0;
	
	if (strcmp(method, "init")   == 0 ||
	    strcmp(method, "initng") == 0 ||
	    strcmp(method, "sysvrc") == 0 ||
	    strcmp(method, "gentoo") == 0)
		return 1;
	
	return 0;
}

int validate_runlevel(char *runlevel)
{
	if (!runlevel)
		return 0;
	
	return str_isalnum(runlevel);
}

int validate_addr(char *addr)
{
	if (!addr)
		return 0;
	
	struct in_addr inaddr;
	return inet_pton(AF_INET, addr, &inaddr);
}

int validate_username(char *username)
{
	if (!username)
		return 0;
	
	return str_isalnum(username);
}

int validate_password(char *password)
{
	if (!password)
		return 0;
	
	return strlen(password) >= 8;
}

int validate_bcap(char *bcap)
{
	if (!bcap)
		return 0;
	
	return flist64_getval(bcaps_list, bcap, NULL) == -1 ? 0 : 1;
}

int validate_ccap(char *ccap)
{
	if (!ccap)
		return 0;
	
	return flist64_getval(ccaps_list, ccap, NULL) == -1 ? 0 : 1;
}

int validate_cflag(char *cflag)
{
	if (!cflag)
		return 0;
	
	return flist64_getval(cflags_list, cflag, NULL) == -1 ? 0 : 1;
}

int validate_rlimit(char *rlimit)
{
	if (!rlimit)
		return 0;
	
	return flist32_getval(rlimit_list, rlimit, NULL) == -1 ? 0 : 1;
}

int validate_rlimits(int soft, int max)
{
	return (max >= soft);
}

int validate_cpuid(int cpuid)
{
	return (cpuid >= 0 && cpuid < 1024);
}

int validate_token_bucket(int32_t fillrate, int32_t interval,
                          int32_t fillrate2, int32_t interval2,
                          int32_t tokensmin, int32_t tokensmax,
                          int32_t priobias)
{
	if (interval < fillrate || interval2 < fillrate2)
		return 0;
	
	if (tokensmin > tokensmax)
		return 0;
	
	return 1;
}

int validate_uname(char *uname)
{
	if (!uname)
		return 0;
	
	return flist32_getval(vhiname_list, uname, NULL) == -1 ? 0 : 1;
}

int validate_uname_value(char *value)
{
	if (str_isempty(value))
		return 0;
	
	return strlen(value) < 65;
}
