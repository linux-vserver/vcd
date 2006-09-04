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
#include <string.h>
#include <strings.h>
#include <vserver.h>

#include <lucid/printf.h>
#include "tools.h"
#include "vlist.h"

#define NAME  "vlimit"
#define DESCR "Context Resource Limit Manager"

#define SHORT_OPTS "SGx:r:l:"

typedef enum { VLIMIT_NONE, VLIMIT_GET, VLIMIT_SET } command_t;

struct options {
	GLOBAL_OPTS;
	command_t cmd;
	xid_t xid;
	uint32_t rid;
	list_t *limit;
};

static inline
void cmd_help()
{
	_lucid_printf("Usage: %s <command> <opts>**\n"
	       "\n"
	       "Available commands:\n"
	       "    -S            Set resource limits\n"
	       "    -G            Get resource limits\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -x <xid>      Context ID\n"
	       "    -r <res>      Use specified resource\n"
	       "    -l <limit>    Set limit specified in <limit>\n"
	       "\n"
	       "Available resource limits:\n"
	       "    RSS, NPROC, NOFILE, MEMLOCK, \n"
	       "    AS, NSOCK, OPENFD, ANON, SHMEM\n"
	       "\n"
	       "Resource limit format string:\n"
	       "    <limit> = <min>,<soft>,<hard>\n"
	       "\n",
	       NAME);
	exit(0);
}

int main(int argc, char *argv[])
{
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.cmd   = VLIMIT_NONE,
		.xid   = 0,
		.rid   = 0,
		.limit = 0,
	};
	
	/* init syscall data */
	struct vx_rlimit rlimit = {
		.id        = 0,
		.minimum   = CRLIM_KEEP,
		.softlimit = CRLIM_KEEP,
		.maximum   = CRLIM_KEEP,
	};
	
	/* init resource limit list */
	list_t *rp = rlimit_list_init();
	
	int c;
	list_node_t *rnode; // resource id search result
	const char delim = ','; // list delimiter

	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'S':
				if (opts.cmd != VLIMIT_NONE)
					cmd_help();
				else
					opts.cmd = VLIMIT_SET;
				break;
			
			case 'G':
				if (opts.cmd != VLIMIT_NONE)
					cmd_help();
				else
					opts.cmd = VLIMIT_GET;
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			case 'r':
				rnode    = list_search(rp, optarg);
				opts.rid = *(uint64_t *)rnode->data;
				break;
			
			case 'l':
				opts.limit = list_parse_list(optarg, delim);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	if ((rlimit.id = opts.rid) == 0)
		EXIT("Invalid resource", EXIT_USAGE);
	
	switch (opts.cmd) {
		case VLIMIT_GET: {
			/* syscall */
			if (vx_get_rlimit(opts.xid, &rlimit) == -1)
				PEXIT("Failed to get resource limits", EXIT_COMMAND);
			
#define PRINTLIMIT(LIMIT, SUFFIX) { \
	if (LIMIT == CRLIM_INFINITY) \
		_lucid_printf("inf" SUFFIX); \
	else \
		_lucid_printf("%llu" SUFFIX, LIMIT); \
}
			/* print list parser compliant MSH value */
			PRINTLIMIT(rlimit.minimum, ",")
			PRINTLIMIT(rlimit.softlimit,  ",")
			PRINTLIMIT(rlimit.maximum, "\n")
#undef PRINTLIMIT
			break;
		}
		case VLIMIT_SET: {
			if (opts.rid == 0 || opts.limit == 0)
				PEXIT("No resource limit specified", EXIT_USAGE);
		
			/* let's make a pointer to prevent unreadable code */
			list_node_t *lnode = (opts.limit)->node;
		
#define SETLIMIT(LIMIT) { \
	if (strlen(lnode->key) != 0) { \
		if (strcasecmp(lnode->key, "inf") == 0) \
			LIMIT = CRLIM_INFINITY; \
		else LIMIT = atoi(lnode->key); \
	} \
	lnode++; \
}
			/* set MSH values */
			SETLIMIT(rlimit.minimum)
			SETLIMIT(rlimit.softlimit)
			SETLIMIT(rlimit.maximum)
#undef SETLIMIT
		
			/* syscall */
			if (vx_set_rlimit(opts.xid, &rlimit) == -1)
				PEXIT("Failed to set resource limits", EXIT_COMMAND);
		
			break;
		}
		default:
			cmd_help();
	}
	exit(EXIT_SUCCESS);
}
