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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <vserver.h>
#include <syslog.h>

#include "tools.h"

static const char *rcsid = "$Id: vdbm.c 160 2006-04-08 13:31:34Z hollow $";

typedef int (*helper_t)(xid_t xid);

static int vshelper_restart(xid_t xid);
static int vshelper_halt(xid_t xid);
static int vshelper_poweroff(xid_t xid);
static int vshelper_swsusp(xid_t xid);
static int vshelper_startup(xid_t xid);
static int vshelper_shutdown(xid_t xid);
static int vshelper_netup(xid_t xid);
static int vshelper_netdown(xid_t xid);

typedef struct {
	char *action;
	helper_t func;
} action_t;

static action_t ACTIONS[] = {
	{ "restart",  vshelper_restart },
	{ "halt",     vshelper_halt },
	{ "poweroff", vshelper_poweroff },
	{ "swsusp",   vshelper_swsusp },
	{ "startup",  vshelper_startup },
	{ "shutdown", vshelper_shutdown },
	{ "netup",    vshelper_netup },
	{ "netdown",  vshelper_netdown },
	{ NULL, NULL }
};

static inline
void usage(int rc)
{
	printf("Usage: vshelper <action> <xid>\n"
	       "\n"
	       "Valid actions:\n"
	       " - restart, halt, poweroff, swsusp\n"
	       " - startup, shutdown\n"
	       " - netup, netdown\n");
	exit(rc);
}

/* restart process:
   1) create new vps killer (background, reboot)
   2) run init-style based stop command
   3) return
*/
static
int vshelper_restart(xid_t xid)
{
	syslog(LOG_INFO, "Received restart event for xid %d", xid);
	return 0;
}

/* halt process:
   1) create new vps killer (background)
   2) run init-style based stop command
   3) return
*/
static
int vshelper_halt(xid_t xid)
{
	syslog(LOG_INFO, "Received halt event for xid %d", xid);
	return 0;
}

/* poweroff process:
   1) halt
*/
static
int vshelper_poweroff(xid_t xid)
{
	syslog(LOG_INFO, "Received poweroff event for xid %d", xid);
	return 0;
}

/* ignore */
static
int vshelper_swsusp(xid_t xid)
{
	syslog(LOG_INFO, "Received swsusp event for xid %d", xid);
	return 0;
}

static
int vshelper_startup(xid_t xid)
{
	syslog(LOG_INFO, "Received startup event for xid %d", xid);
	return 0;
}

static
int vshelper_shutdown(xid_t xid)
{
	syslog(LOG_INFO, "Received shutdown event for xid %d", xid);
	return 0;
}

static
int vshelper_netup(xid_t xid)
{
	syslog(LOG_INFO, "Received netup event for xid %d", xid);
	return 0;
}

static
int vshelper_netdown(xid_t xid)
{
	syslog(LOG_INFO, "Received netdown event for xid %d", xid);
	return 0;
}

int main(int argc, char *argv[])
{
	INIT_ARGV0
	int i;
	
	if (argc != 3)
		usage(EXIT_FAILURE);
	
	char *action = argv[1];
	xid_t xid    = atoi(argv[2]);
	
	if (xid < 2)
		err("xid must be >= 2");
	
	for (i = 0; ACTIONS[i].action; i++)
		if (strcmp(ACTIONS[i].action, action) == 0)
			return ACTIONS[i].func(xid);
	
	err("invalid action: %s", action);
}
