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
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <vserver.h>

#include <lucid/printf.h>
#include "tools.h"
#include "vlist.h"

#define NAME  "vflags"
#define DESCR "Context Flag/Capability Manager"

#define SHORT_OPTS "SGLx:b:c:f:rm"

typedef enum { VFLAG_NONE, VFLAG_GET, VFLAG_SET, VFLAG_LIST} command_t;

struct options {
	GLOBAL_OPTS;
	command_t cmd;
	xid_t xid;
	bool reset;
	bool compact;
	list_t *bcaps;
	list_t *ccaps;
	list_t *flags;
};

static inline
void cmd_help()
{
	_lucid_printf("Usage: %s <command> <opts>*\n"
	       "\n"
	       "Available commands:\n"
	       "    -S            Set context capabilities/flags\n"
	       "    -G            Get context capabilities/flags\n"
	       "    -L            List available capabilities/flags\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -x <xid>      Context ID\n"
	       "    -b <list>     Set capabilities described in <list>\n"
	       "                  (You can only lower bcaps!)\n"
	       "    -c <list>     Set capabilities described in <list>\n"
	       "    -f <list>     Set flags described in <list>\n"
	       "    -r            Ignore currently set bcaps\n"
	       "    -m            Display caps/flags as list\n"
	       "\n"
	       "Flag list format string:\n"
	       "    <list> = [~]<flag>,[~]<flag>,...\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.cmd   = VFLAG_NONE,
		.xid   = 0,
		.bcaps = 0,
		.ccaps = 0,
		.flags = 0,
		.reset = 0,
		.compact = 0,
	};
	
	/* init syscall data */
	struct vx_flags flags = {
		.flags = 0,
		.mask  = 0,
	};
	struct vx_ccaps ccaps = {
		.ccaps = 0,
		.cmask = 0,
	};
	struct vx_bcaps bcaps = {
		.bcaps = 0,
		.bmask = 0,
	};

	/* init capability lists */
	list_t *bp = bcaps_list_init();
	list_t *cp = ccaps_list_init();
	list_t *fp = cflags_list_init();
	
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
			
			case 'S':
				if (opts.cmd != VFLAG_NONE)
					cmd_help();
				else
					opts.cmd = VFLAG_SET;
				break;
			
			case 'G':
				if (opts.cmd != VFLAG_NONE)
					cmd_help();
				else
					opts.cmd = VFLAG_GET;
				break;
			
			case 'L':
				if (opts.cmd != VFLAG_NONE)
					cmd_help();
				else
					opts.cmd = VFLAG_LIST;
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			case 'b':
				opts.bcaps = list_parse_list(optarg, delim);
				break;
			
			case 'c':
				opts.ccaps = list_parse_list(optarg, delim);
				break;
			
			case 'f':
				opts.flags = list_parse_list(optarg, delim);
				break;
			
			case 'r':
				opts.reset = 1;
				break;
				
			case 'm':
				opts.compact = 1;
				break;
				
			DEFAULT_GETOPT
		}
	}

	switch (opts.cmd) {
		case VFLAG_SET:
			if (opts.bcaps) {
				list_link_t blink = {
					.p = bp,
					.d = opts.bcaps,
				};

				/* validate descending list */
				if (list_validate_flag(&blink, clmod) == -1)
					PEXIT("BCaps list validation failed", EXIT_USAGE);

				/* convert given descending list to flags using the pristine copy */
				list_list2flags(&blink, clmod, &bcaps.bcaps, &bcaps.bmask);

				DEBUGF("BCaps = %.16llx, mask  = %.16llx\n", bcaps.bcaps, bcaps.bmask);
				if (bcaps.bmask != 0 && vx_set_bcaps(opts.xid, &bcaps) == -1)
					PEXIT("Failed to set system capabilities", EXIT_COMMAND);
			}

			if (opts.ccaps) {
				list_link_t clink = {
					.p = cp,
					.d = opts.ccaps,
				};

				/* validate descending lists */
				if (list_validate_flag(&clink, clmod) == -1)
					PEXIT("CCaps list validation failed", EXIT_USAGE);

				/* convert given descending list to flags using the pristine copy */
				list_list2flags(&clink, clmod, &ccaps.ccaps, &ccaps.cmask);

				DEBUGF("CCaps = %.16llx, mask = %.16llx\n", ccaps.ccaps, ccaps.cmask);
				if (ccaps.cmask != 0 && vx_set_ccaps(opts.xid, &ccaps) == -1)
					PEXIT("Failed to set context capabilities", EXIT_COMMAND);
			}

			if (opts.flags) {
				list_link_t flink = {
					.p = fp,
					.d = opts.flags,
				};

				/* validate descending list */
				if (list_validate_flag(&flink, clmod) == -1)
					PEXIT("List validation failed", EXIT_USAGE);

				/* convert given descending list to flags using the pristine copy */
				list_list2flags(&flink, clmod, &flags.flags, &flags.mask);

				/* syscall */
				if (flags.mask != 0 && vx_set_flags(opts.xid, &flags) == -1)
					PEXIT("Failed to set context flags", EXIT_COMMAND);
			}
			break;

		case VFLAG_GET: {
			/* syscall */
			if (vx_get_bcaps(opts.xid, &bcaps) == -1)
				PEXIT("Failed to get system capabilities", EXIT_COMMAND);
			if (vx_get_ccaps(opts.xid, &ccaps) == -1)
				PEXIT("Failed to get context capabilities", EXIT_COMMAND);
			if (vx_get_flags(opts.xid, &flags) == -1)
				PEXIT("Failed to get context flags", EXIT_COMMAND);

			VPRINTF(&opts, "Flags = %.16llx\n", flags.flags);
			VPRINTF(&opts, "BCaps = %.16llx\n", bcaps.bcaps);
			VPRINTF(&opts, "CCaps = %.16llx\n", ccaps.ccaps);

			if (opts.compact) {
				int b = 0;
				_lucid_printf("BCAPS =");
				list_foreach(bp, i)
					if ((bcaps.bcaps & *(uint64_t*)(bp->node+i)->data) == *(uint64_t*)(bp->node+i)->data)
						_lucid_printf("%s%s", b++ ? "," : " ", (char *)(bp->node+i)->key);
				_lucid_printf("\nCCAPS =");
				b = 0;
				list_foreach(cp, i)
					if (ccaps.ccaps & *(uint64_t*)(cp->node+i)->data)
						_lucid_printf("%s%s", b++ ? "," : " ", (char *)(cp->node+i)->key);
				_lucid_printf("\nFLAGS =");
				b = 0;
				list_foreach(fp, i)
					if (flags.flags & *(uint64_t*)(fp->node+i)->data)
						_lucid_printf("%s%s", b++ ? "," : " ", (char *)(fp->node+i)->key);
				_lucid_printf("\n");
			} else {
				/* iterate through each list and print matching keys */
				list_foreach(bp, i) {
					if ((bcaps.bcaps & *(uint64_t*)(bp->node+i)->data) == *(uint64_t*)(bp->node+i)->data)
						_lucid_printf("B: %s\n", (char *)(bp->node+i)->key);
				}

				list_foreach(cp, i) {
					if (ccaps.ccaps & *(uint64_t*)(cp->node+i)->data)
						_lucid_printf("C: %s\n", (char *)(cp->node+i)->key);
				}

				list_foreach(fp, i) {
					if (flags.flags & *(uint64_t*)(fp->node+i)->data)
						_lucid_printf("F: %s\n", (char *)(fp->node+i)->key);
				}
			}
			break;
		}
		case VFLAG_LIST: {
			/* iterate through each list and print all keys */
			list_foreach(bp, i)
				_lucid_printf("B: %s\n", (char *)(bp->node+i)->key);

			list_foreach(cp, i)
				_lucid_printf("C: %s\n", (char *)(cp->node+i)->key);

			list_foreach(fp, i)
				_lucid_printf("F: %s\n", (char *)(fp->node+i)->key);

			break;
		}
		default:
			cmd_help();
	}

	exit(EXIT_SUCCESS);
}
