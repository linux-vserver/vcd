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

#include <lucid/printf.h>
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
	uint64_t flags;
	uint64_t fmask;
	uint64_t bcaps;
	uint64_t bmask;
	uint64_t ccaps;
	uint64_t cmask;
	uint64_t mflags;
	uint64_t mmask;
};

static inline
void cmd_help()
{
	_lucid_printf("Usage: %s <command> <opts>* -- <program> <args>*\n"
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
	       "    -m <list>     Set migration flags described in <list>\n"
	       "\n"
	       "Flag list format string:\n"
	       "    <list> = [~]<flag>,[~]<flag>,...\n"
	       "\n"
	       "    See 'vflags -L' for available flags\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

#define delim ',' // list delimiter
#define clmod '~' // clear flag modifier

int calc_caps(const char*lst, list_t *vals, uint64_t *flags, uint64_t *fmask)
{
	list_link_t link = {
		.p = vals,
		.d = list_parse_list(lst, delim),
	};
	
	/* validate descending list */
	if (list_validate_flag(&link, clmod) == -1)
		return -1;
	
	/* convert given descending list to flags using the pristine copy */
	list_list2flags(&link, clmod, flags, fmask);
	return 0;
}

int main(int argc, char *argv[])
{
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.cmd    = VCTX_NONE,
		.xid    = (~0U),
		.flags  = 0,
		.fmask  = 0,
		.ccaps  = 0,
		.cmask  = 0,
		.bcaps  = 0,
		.bmask  = 0,
		.mflags = 0,
		.mmask  = 0,
	};
	
	struct vx_info info;
	
	list_t *fp = cflags_list_init();
	list_t *bp = bcaps_list_init();
	list_t *cp = ccaps_list_init();
	list_t *mp = mflags_list_init();
	
	int c;
	
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
				if (calc_caps(optarg, fp, &opts.flags, &opts.fmask) == -1)
					PEXIT("List validation failed for FLAGS", EXIT_USAGE);
				break;
			
			case 'c':
				if (calc_caps(optarg, cp, &opts.ccaps, &opts.cmask) == -1)
					PEXIT("List validation failed for CCAPS", EXIT_USAGE);
				break;
				
			case 'b':
				if (calc_caps(optarg, bp, &opts.bcaps, &opts.bmask) == -1)
					PEXIT("List validation failed for BCAPS", EXIT_USAGE);
				break;
				
			case 'm':
				if (calc_caps(optarg, mp, &opts.mflags, &opts.mmask) == -1)
					PEXIT("List validation failed for MFLAGS", EXIT_USAGE);
				break;
			
			DEFAULT_GETOPT
		}
	}

	switch (opts.cmd) {
		case VCTX_CREATE: {
			struct vx_create_flags create_flags = {
				.flags = VXF_STATE_SETUP | VXF_STATE_ADMIN | opts.flags,
			};

			/* syscall */
			if (vx_create(opts.xid, &create_flags) == -1)
				PEXIT("Failed to create context", EXIT_COMMAND);

			/* Set as default values those used by util-vserver */
			uint64_t defcaps;
			uint64_t defmcaps;

			/* Set context capabilities ... */
			struct vx_ccaps ccaps = {
				.ccaps = 0,
				.cmask = 0,
			};
			if (calc_caps("SET_UTSNAME,RAW_ICMP", cp, &defcaps, &defmcaps) == -1)
				PEXIT("List validation failed for default CCAPS", EXIT_USAGE);
			ccaps.ccaps = (opts.ccaps & opts.cmask) | (defcaps & defmcaps & ~opts.cmask);
			ccaps.cmask = opts.cmask | defmcaps;
			if (vx_set_ccaps(opts.xid, &ccaps) == -1)
				PEXIT("Failed to set context capabilities", EXIT_COMMAND);

			/* Set context system capabilities */
			struct vx_bcaps bcaps = {
				.bcaps = 0,
				.bmask = 0,
			};
			if (calc_caps("CHOWN,DAC_OVERRIDE,DAC_READ_SEARCH,"
					"FOWNER,FSETID,FS_MASK,KILL,SETGID,SETUID,NET_BIND_SERVICE,SYS_CHROOT,"
					"SYS_PTRACE,SYS_BOOT,SYS_TTY_CONFIG,LEASE", bp, &defcaps, &defmcaps) == -1)
				PEXIT("List validation failed for default BCAPS", EXIT_USAGE);
			bcaps.bcaps = (opts.bcaps & opts.bmask) | (defcaps & defmcaps & ~opts.bmask);
			bcaps.bmask = ~0l;
			if (vx_set_bcaps(opts.xid, &bcaps) == -1)
				PEXIT("Failed to set system capabilities", EXIT_COMMAND);


			if (argc > optind) {
				struct vx_flags flags = {
					.flags = 0,
					.mask  = VXF_STATE_SETUP | VXF_STATE_ADMIN,
				};
				if (vx_set_flags(opts.xid, &flags) == -1)
					PEXIT("Failed to set context flags", EXIT_COMMAND);
			}
			break;
		}
		case VCTX_MIGRATE: {
			struct vx_migrate_flags migrate_flags = {
				.flags = opts.mflags,
			};

			/* syscall */
			if (vx_migrate(opts.xid, &migrate_flags) == -1)
				PEXIT("Failed to migrate to context", EXIT_COMMAND);
			break;
		}
		case VCTX_INFO: {
			/* syscall */
			if (vx_get_info(opts.xid, &info) == -1)
				PEXIT("Failed to get context information", EXIT_COMMAND);

			_lucid_printf("Context ID: %d\n", info.xid);
			_lucid_printf("Init PID: %d\n", info.initpid);

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
