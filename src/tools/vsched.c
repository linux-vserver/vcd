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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <vserver.h>

#include "printf.h"
#include "tools.h"
#include "vlist.h"

#define NAME  "vsched"
#define DESCR "Context CPU Limit Manager"

#define SHORT_OPTS "x:b:"

struct options {
	GLOBAL_OPTS;
	xid_t xid;
	list_t *bucket;
};

void cmd_help()
{
	vu_printf("Usage: %s <opts>*\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -x <xid>      Context ID\n"
	       "    -b <bucket>   Set bucket described in <bucket>\n"
	       "\n"
	       "Bucket format string:\n"
	       "    <bucket> = <key>=<value>,<key>=<value>,...\n"
	       "\n"
	       "    <key> is one of: FILL_RATE, INTERVAL, TOKENS, \n"
	       "                     TOKENS_MIN, TOKENS_MAX, PRIO_BIAS\n"
	       "    <value> must be an integer\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.xid    = 0,
		.bucket = 0,
	};
	
	/* init syscall data */
	struct vx_sched sched = {
		.set_mask      = 0,
		.fill_rate     = 0,
		.interval      = 0,
		.tokens        = 0,
		.tokens_min    = 0,
		.tokens_max    = 0,
		.prio_bias     = 0,
		.cpu_id        = 0,
		.bucket_id     = 0,
	};
	
	int c;
	const char delim   = ','; // list delimiter
	const char kvdelim = '='; // key/value delimiter
	
	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			case 'b':
				opts.bucket = list_parse_hash(optarg, delim, kvdelim);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	if (opts.bucket != 0) {
		/* init scheduler mask list */
		list_t *sp = sched_list_init();
		
		list_link_t link = {
			.p = sp,
			.d = opts.bucket,
		};
		
		/* validate descending list */
		if (list_validate(&link) == -1)
			PEXIT("List validation failed", EXIT_USAGE);
		
		/* we kinda misuse the list parser here:
		 * the pristine list is used to calculate the set mask
		 * whereas the descending list is used to set the actual values
		 */
		list_foreach(link.d, i) {
			/* let's make a pointer to prevent unreadable code */
			list_node_t *dnode = (link.d)->node+i;
			
			/* find set mask to given key from descending list */
			list_node_t *pnode = list_search(link.p, dnode->key);
			
			/* convert list data */
			char *key      = pnode->key;
			uint64_t value = atoi((char *)dnode->data);
			uint64_t mask  = *(uint64_t *)pnode->data;
			
			/* set value of d according to key from p */
			if (strcasecmp(key, "FILL_RATE") == 0)
				sched.fill_rate = value;
			
			if (strcasecmp(key, "INTERVAL") == 0)
				sched.interval = value;
			
			if (strcasecmp(key, "TOKENS") == 0)
				sched.tokens = value;
			
			if (strcasecmp(key, "TOKENS_MIN") == 0)
				sched.tokens_min = value;
			
			if (strcasecmp(key, "TOKENS_MAX") == 0)
				sched.tokens_max = value;
			
			if (strcasecmp(key, "PRIO_BIAS") == 0)
				sched.prio_bias = value;
		
			if (strcasecmp(key, "CPU_ID") == 0)
				sched.cpu_id = value;

			if (strcasecmp(key, "BUCKET_ID") == 0)
				sched.bucket_id = value;
			
			/* set scheduler mask */
			sched.set_mask |= mask;
		}
		
		/* syscall */
		if (vx_set_sched(opts.xid, &sched) == -1)
			PEXIT("Failed to set scheduler settings", EXIT_COMMAND);
		
		goto out;
	}
	
	cmd_help();

out:
	exit(EXIT_SUCCESS);
}
