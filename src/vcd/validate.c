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
#include <dirent.h>

#include "lists.h"
#include "validate.h"

#include <lucid/addr.h>
#include <lucid/log.h>
#include <lucid/scanf.h>
#include <lucid/str.h>
#include <lucid/strtok.h>

static
int max_mem_node(void)
{
	DIR *ndir;
	struct dirent *dent;
	int maxmemnode = 0;

	ndir = opendir("/sys/devices/system/node");
	if (!ndir)
		return 0;

	while ((dent = readdir(ndir))) {
		if (str_cmpn(dent->d_name, "node", 4))
			continue;

		int node;
		sscanf(dent->d_name+4, "%d", &node);

		if (maxmemnode < node)
			maxmemnode = node;
	}

	closedir(ndir);

	return maxmemnode;
}

static
int str_isalnumextended(const char *str)
{
	int i, n;

	if (!str)
		return 1;

	n = str_len(str);

	for (i = 0; i < n; i++) {
		if ((unsigned int)(str[i] - 'a') < 26u || (unsigned int)(str[i] - 'A') < 26u || (unsigned int)(str[i] - '0') < 10u
			|| str[i] == '-' || str[i] == '_') continue;
		return 0;
	}

	return 1;
}


int validate_name(const char *name)
{
	LOG_TRACEME
	return !(str_isempty(name) || str_len(name) < 3 ||
			str_len(name) > 32 || !str_isalnumextended(name));
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

int validate_group(const char *group)
{
	LOG_TRACEME
	return !(str_isempty(group) || !str_isalnum(group));
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
	return (cpuid >= -1 && cpuid < sysconf(_SC_NPROCESSORS_ONLN));
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

int validate_cpuset(const char *cpuset, int type)
{
	LOG_TRACEME

	if (str_isempty(cpuset))
		return 0;

	int cpusetlen = str_len(cpuset)-1;

	if (str_cmpn(cpuset, ",", 1) == 0 || str_cmpn(cpuset+cpusetlen, ",", 1) == 0)
		return 0;

	strtok_t _st, *st = &_st, *p;

	if (!strtok_init_str(st, cpuset, ",", 0))
		return 0;

	int maxcpus = 0, maxmems = 0;

	if (type == 0)
		maxcpus = sysconf(_SC_NPROCESSORS_ONLN);
	else if (type == 1)
		maxmems = max_mem_node();
	else
		return 0;

	int errorout = 0;

	strtok_for_each(st, p) {
		if (!str_isdigit(p->token)) {
			errorout = 1;
			break;
		}

		int id;
		sscanf(p->token, "%d", &id);

		if (type == 0) {
			if (!(id >= 0 && id < maxcpus)) {
				errorout = 1;
				break;
			}
		}

		else if (type == 1) {
			if (!(id >= 0 && id <= maxmems)) {
				errorout = 1;
				break;
			}
		}

		else {
			errorout = 1;
			break;
		}
	}

	strtok_free(st);

	return (errorout == 0) ? 1 : 0;
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
