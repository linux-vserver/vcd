/***************************************************************************
 *   Copyright 2005 by the vserver-utils team                              *
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

#include <stdlib.h>

#include "vc.h"
#include "commands.h"

static const char *start_rcsid = "$Id$";

/* global storage for cleanup handler */
static int start_state = 0;
static char *start_name = NULL;

/* setup states */
#define S_HAVE_VX 0x00000001
#define S_HAVE_NX 0x00000002
#define S_HAVE_NS 0x00000004

#define S_DONE    0x80000000

int start_usage(int rc)
{
	vc_printf("start: Start a virtual server.\n"
	          "usage: start <name>\n"
	          "\n"
	          "%s\n", start_rcsid);
	
	exit(rc);
}

static
void start_cleanup(void)
{
	if (start_state & S_DONE)
		return;
	
	if (start_state & S_HAVE_NX)
		vc_nx_release(start_name);
	
	if (start_state & S_HAVE_VX)
		vc_vx_release(start_name);
}

static
void start_sighandler(int sig)
{
	signal(sig, SIG_DFL);
	start_cleanup();
	kill(getpid(), sig);
}

int start_main(int argc, char *argv[])
{
	VC_INIT_ARGV0
	
	if (argc < 2)
		return start_usage(EXIT_FAILURE);
	
	/* save name for cleanup handler */
	start_name = argv[1];
	
	/* global exit handler */
	atexit(start_cleanup);
	
	/* catch signals */
	signal(SIGHUP,  start_sighandler);
	signal(SIGINT,  start_sighandler);
	signal(SIGQUIT, start_sighandler);
	signal(SIGABRT, start_sighandler);
	signal(SIGSEGV, start_sighandler);
	signal(SIGTERM, start_sighandler);
	
	/* create context */
	if (vc_vx_new(start_name, NULL) == -1)
		vc_errp("vc_vx_new(%s)", start_name);
	
	start_state |= S_HAVE_VX;
	
	/* create network context */
	if (vc_nx_new(start_name, NULL) == -1)
		vc_errp("vc_nx_new(%s)", start_name);
	
	start_state |= S_HAVE_NX;
	
	/* create new filesystem namespace */
	if (vc_ns_new(start_name) == -1)
		vc_errp("vc_ns_new(%s)", start_name);
	
	start_state |= S_HAVE_NS;
	
	/* we're done now */
	start_state |= S_DONE;
	
	return EXIT_SUCCESS;
}
