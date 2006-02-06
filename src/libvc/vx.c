/***************************************************************************
 *   Copyright 2005-2006 by the vserver-utils team                         *
 *   See AUTHORS for details                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <wait.h>

#include "printf.h"
#include "vconfig.h"
#include "vc.h"


int vc_vx_exists(char *name)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_info info;
	
	return vx_get_info(xid, &info) == -1 ? 0 : 1;
}

int vc_vx_create(char *name, char *flagstr)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_create_flags flags = {
		.flags = 0,
	};
	
	if (flagstr == NULL)
		goto create;
	
	uint64_t mask = 0;
	
	if (vc_list64_parse(flagstr, vc_cflags_list, &flags.flags, &mask, '~', ',') == -1)
		return -1;
	
create:
	if (vx_create(xid, &flags) == -1)
		return -1;
	
	return 0;
}

int vc_vx_new(char *name, char *flagstr)
{
	pid_t pid;
	int status;
	char *buf;
	
	switch((pid = fork())) {
		case -1:
			return -1;
		
		case 0:
			vc_asprintf(&buf, "%s,%s", flagstr, "PERSISTANT");
			
			if (vc_vx_create(name, buf) == -1)
				exit(EXIT_FAILURE);
			else
				exit(EXIT_SUCCESS);
		
		default:
			if (waitpid(pid, &status, 0) == -1)
				return -1;
		
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) == EXIT_SUCCESS)
					return 0;
				else
					return -1;
			}
			
			if (WIFSIGNALED(status)) {
				kill(getpid(), WTERMSIG(status));
				exit(EXIT_FAILURE);
			}
	}
	
	return 0;
}

int vc_vx_migrate(char *name)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	if (!vc_vx_exists(name)) {
		if (vc_vx_create(name, NULL) == -1)
			return -1;
	}
	
	else {
		if (vx_migrate(xid) == -1)
			return -1;
	}
	
	return 0;
}

int vc_vx_kill(char *name, pid_t pid, int sig)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_kill_opts kill_opts = {
		.pid = pid,
		.sig = sig,
	};
	
	if (vx_kill(xid, &kill_opts) == -1)
		return -1;
	
	return 0;
}

int vc_vx_wait(char *name)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_wait_opts wait_opts = {
		.a = 0,
		.b = 0,
	};
	
	if (vx_wait(xid, &wait_opts) == -1)
		return -1;
	
	return 0;
}

int vc_vx_get_bcaps(char *name, char **flagstr, uint64_t *flags)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_caps caps = {
		.bcaps = 0,
		.bmask = 0,
		.ccaps = 0,
		.cmask = 0,
	};
	
	if (vx_get_caps(xid, &caps) == -1)
		return -1;
	
	if (flags != NULL)
		*flags = caps.bcaps;
	
	if (vc_list64_tostr(vc_bcaps_list, caps.bcaps, flagstr, ',') == -1)
		return -1;
	
	return 0;
}

int vc_vx_get_ccaps(char *name, char **flagstr, uint64_t *flags)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_caps caps = {
		.bcaps = 0,
		.bmask = 0,
		.ccaps = 0,
		.cmask = 0,
	};
	
	if (vx_get_caps(xid, &caps) == -1)
		return -1;
	
	if (flags != NULL)
		*flags = caps.ccaps;
	
	if (vc_list64_tostr(vc_ccaps_list, caps.ccaps, flagstr, ',') == -1)
		return -1;
	
	return 0;
}

int vc_vx_get_flags(char *name, char **flagstr, uint64_t *flags)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_flags cflags = {
		.flags = 0,
		.mask  = 0,
	};
	
	if (vx_get_flags(xid, &cflags) == -1)
		return -1;
	
	if (flags != NULL)
		*flags = cflags.flags;
	
	if (vc_list64_tostr(vc_cflags_list, cflags.flags, flagstr, ',') == -1)
		return -1;
	
	return 0;
}

int vc_vx_get_limit(char *name, char *type,
                    uint64_t *min, uint64_t *soft, uint64_t *max)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_rlimit rlimit = {
		.id        = 0,
		.minimum   = 0,
		.softlimit = 0,
		.maximum   = 0,
	};
	
	if (vc_list32_getval(vc_rlimit_list, type, &rlimit.id) == -1)
		return -1;
	
	if (vx_get_rlimit(xid, &rlimit) == -1)
		return -1;
	
	*min  = rlimit.minimum;
	*soft = rlimit.softlimit;
	*max  = rlimit.maximum;
	
	return 0;
}

int vc_vx_get_uname(char *name, char *key, char **value)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	uint32_t field;
	
	if (vc_list32_getval(vc_vhiname_list, key, &field) == -1)
		return -1;
	
	struct vx_vhi_name vhi_name;
	vhi_name.field = field;
	
	if (vx_get_vhi_name(xid, &vhi_name) == -1)
		return -1;
	
	vc_asprintf(value, "%s", vhi_name.name);
	
	return 0;
}


int vc_vx_set_bcaps(char *name, char *flagstr)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_caps caps = {
		.bcaps = ~(0ULL),
		.bmask = ~(0ULL),
		.ccaps = 0,
		.cmask = 0,
	};
	
	if (flagstr != NULL)
		if (vc_list64_parse(flagstr, vc_bcaps_list, &caps.bcaps, &caps.bmask, '~', ',') == -1)
			return -1;
	
	if (vx_set_caps(xid, &caps) == -1)
		return -1;
	
	return 0;
}

int vc_vx_set_ccaps(char *name, char *flagstr)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_caps caps = {
		.bcaps = ~(0ULL),
		.bmask = ~(0ULL),
		.ccaps = 0,
		.cmask = 0,
	};
	
	if (vc_list64_parse(flagstr, vc_ccaps_list, &caps.ccaps, &caps.cmask, '~', ',') == -1)
		return -1;
	
	if (vx_set_caps(xid, &caps) == -1)
		return -1;
	
	return 0;
}

int vc_vx_set_flags(char *name, char *flagstr)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	struct vx_flags flags = {
		.flags = 0,
		.mask  = 0,
	};
	
	if (vc_list64_parse(flagstr, vc_cflags_list, &flags.flags, &flags.mask, '~', ',') == -1)
		return -1;
	
	if (vx_set_flags(xid, &flags) == -1)
		return -1;
	
	return 0;
}

int vc_vx_set_limit(char *name, char *type,
                    uint32_t min, uint32_t soft, uint32_t max)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	uint32_t id;
	
	if (vc_list32_getval(vc_rlimit_list, type, &id) == -1)
		return -1;
	
	struct vx_rlimit rlimit = {
		.id        = id,
		.minimum   = min,
		.softlimit = soft,
		.maximum   = max,
	};
	
	if (vx_set_rlimit(xid, &rlimit) == -1)
		return -1;
	
	return 0;
}

int vc_vx_set_uname(char *name, char *key, char *value)
{
	xid_t xid;
	
	if (vconfig_get_xid(name, &xid) == -1)
		return -1;
	
	uint32_t field;
	
	if (vc_list32_getval(vc_vhiname_list, key, &field) == -1)
		return -1;
	
	struct vx_vhi_name vhi_name;
	vhi_name.field = field;
	vc_snprintf(vhi_name.name, 65, "%s", value);
	
	if (vx_set_vhi_name(xid, &vhi_name) == -1)
		return -1;
	
	return 0;
}
