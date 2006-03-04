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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <vserver.h>

#include "printf.h"
#include "tools.h"
#include "vlist.h"

#define NAME  "vcontext"
#define DESCR "Context Manager"

#define SHORT_OPTS "CIMx:f:c:b:m"

typedef enum { VCTX_NONE, VCTX_CREATE, VCTX_INFO, VCTX_MIGRATE } command_t;

struct options {
	GLOBAL_OPTS;
	command_t cmd;
	xid_t xid;
	list_t *flags;
	list_t *bcaps;
	list_t *ccaps;
	list_t *mflags;
};

static inline
void cmd_help()
{
	vu_printf("Usage: %s <command> <opts>* -- <program> <args>*\n"
	       "\n"
	       "Available commands:\n"
	       "    -C            Create a new security context\n"
	       "    -I            Get information about a context\n"
	       "    -M            Migrate to an existing context\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -x <xid>      Context ID\n"
	       "    -f <list>     Set flags described in <list>\n"
	       "    -b <list>     Set bcaps described in <list>\n"
	       "    -c <list>     Set ccaps described in <list>\n"
/*	       "    -m <list>     Set migration flags described in <list>\n" */
	       "\n"
	       "Flag list format string:\n"
	       "    <list> = [~]<flag>,[~]<flag>,...\n"
	       "\n"
	       "    See 'vflags -L' for available flags\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.cmd   = VCTX_NONE,
		.xid   = (~0U),
		.flags = 0,
	};
	
	struct vx_info info;
	
	int c;
	const char delim = ','; // list delimiter
	const char clmod = '~'; // clear flag modifier
	
	DEBUGF("%s: starting ...\n", NAME);
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'C':
				if (opts.cmd != VCTX_NONE)
					cmd_help();
				else
					opts.cmd = VCTX_CREATE;
				break;
			
			case 'I':
				if (opts.cmd != VCTX_NONE)
					cmd_help();
				else
					opts.cmd = VCTX_INFO;
				break;
			
			case 'M':
				if (opts.cmd != VCTX_NONE)
					cmd_help();
				else
					opts.cmd = VCTX_MIGRATE;
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			case 'f':
				opts.flags = list_parse_list(optarg, delim);
				break;
			
			case 'c':
				opts.ccaps = list_parse_list(optarg, delim);
				break;
				
			case 'b':
				opts.bcaps = list_parse_list(optarg, delim);
				break;
				
			case 'm':
				opts.mflags = list_parse_list(optarg, delim);
				break;
			
			DEFAULT_GETOPT
		}
	}

	switch (opts.cmd) {
		case VCTX_CREATE: {
			struct vx_create_flags create_flags = {
				.flags = VXF_STATE_SETUP,
			};
			if (opts.flags) {
				/* init flags list */
				list_t *cp = cflags_list_init();

				list_link_t link = {
					.p = cp,
					.d = opts.flags,
				};
				uint64_t fmask = 0;
				
				/* validate descending list */
				if (list_validate_flag(&link, clmod) == -1)
					PEXIT("List validation failed for FLAGS", EXIT_USAGE);
			
				/* convert given descending list to flags using the pristine copy */
				list_list2flags(&link, clmod, &create_flags.flags, &fmask);
				create_flags.flags |= VXF_STATE_SETUP;
			}
			
			/* syscall */
			if (vx_create(opts.xid, &create_flags) == -1)
				PEXIT("Failed to create context", EXIT_COMMAND);
			
			/* Set as default values those used by util-vserver */
			if (opts.ccaps == NULL) opts.ccaps = list_parse_list("SET_UTSNAME,RAW_ICMP", delim);
			if (opts.bcaps == NULL) opts.bcaps = list_parse_list("CHOWN,DAC_OVERRIDE,DAC_READ_SEARCH,"
					"FOWNER,FSETID,FS_MASK,KILL,SETGID,SETUID,NET_BIND_SERVICE,SYS_CHROOT,"
					"SYS_PTRACE,SYS_BOOT,SYS_TTY_CONFIG,LEASE,SYS_ADMIN", delim);
			if (opts.ccaps || opts.bcaps) {
				struct vx_caps caps = {
					.bcaps = 0,
					.ccaps = 0,
					.cmask = 0,
				};
				list_t *bp = bcaps_list_init();
				list_t *cp = ccaps_list_init();
				
				list_link_t link = {
					.p = bp,
					.d = opts.bcaps,
				};

				/* validate descending list */
				if (list_validate_flag(&link, clmod) == -1)
					PEXIT("List validation failed for BCAPS", EXIT_USAGE);
				/* convert given descending list to flags using the pristine copy */
				list_list2flags(&link, clmod, &caps.bcaps, &caps.cmask);

				link.p = cp;
				link.d = opts.ccaps;
				/* validate descending list */
				if (list_validate_flag(&link, clmod) == -1)
					PEXIT("List validation failed for CCAPS", EXIT_USAGE);
				/* convert given descending list to flags using the pristine copy */
				list_list2flags(&link, clmod, &caps.ccaps, &caps.cmask);

				/* Finally set the caps */
				if (vx_set_caps(opts.xid, &caps) == -1)
					PEXIT("Failed to set context capabilities", EXIT_COMMAND);
			}
			if (argc > optind) {
				struct vx_flags flags = {
					.flags = 0,
					.mask  = VXF_STATE_SETUP,
				};
				if (vx_set_flags(opts.xid, &flags) == -1)
					PEXIT("Failed to set context flags", EXIT_COMMAND);
			}
			break;
		}
		case VCTX_MIGRATE: {
			struct vx_migrate_flags migrate_flags = {
				.flags = 0,
			};

			if (opts.mflags) {
				/* init flags list */
				list_t *mp = mflags_list_init();
				
				list_link_t link = {
					.p = mp,
					.d = opts.mflags,
				};
				uint64_t fmask = 0;
				
				/* validate descending list */
				if (list_validate_flag(&link, clmod) == -1)
					PEXIT("List validation failed", EXIT_USAGE);
				
				/* convert given descending list to flags using the pristine copy */
				list_list2flags(&link, clmod, &migrate_flags.flags, &fmask);
			}
			/* syscall */
			if (vx_migrate(opts.xid, &migrate_flags) == -1)
				PEXIT("Failed to migrate to context", EXIT_COMMAND);
			break;
		}
		case VCTX_INFO: {
			/* syscall */
			if (vx_get_info(opts.xid, &info) == -1)
				PEXIT("Failed to get context information", EXIT_COMMAND);
		
			vu_printf("Context ID: %d\n", info.xid);
			vu_printf("Init PID: %d\n", info.initpid);
		
			goto out;
		}
		default:
			cmd_help();
	}
	
	if (argc > optind)
		execvp(argv[optind], argv+optind);
	
out:
	exit(EXIT_SUCCESS);
}
