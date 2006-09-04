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
#include <vserver.h>

#include <lucid/printf.h>
#include "tools.h"
#include "vlist.h"

#define NAME  "vuname"
#define DESCR "Virtual Host Information Manager"

#define SHORT_OPTS "SGx:n:"

typedef enum { VUN_NONE, VUN_GET, VUN_SET } command_t;

struct options {
	GLOBAL_OPTS;
	command_t cmd;
	xid_t xid;
	list_t *names;
};

static inline
void cmd_help()
{
	_lucid_printf("Usage: %s <command> <opts>*\n"
	       "\n"
	       "Available commands:\n"
	       "    -S            Set virtual host information\n"
	       "    -G            Get virtual host information\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -x <xid>      Context ID\n"
	       "    -n <names>    Set VHI names described in <names>\n"
	       "\n"
	       "VHI names format string:\n"
	       "    <names> = <key>=<value>,<key>=<value>,...\n"
	       "\n"
	       "    <key> is one of: CONTEXT, SYSNAME, NODENAME, RELEASE,\n"
	       "                     VERSION, MACHINE, DOMAINNAME\n"
	       "    <value> must be a string\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.cmd   = VUN_NONE,
		.xid   = 0,
		.names = 0,
	};
	
	/* init syscall data */
	struct vx_vhi_name vhi_name;
	
	/* init vhi names list */
	list_t *vp = vhi_list_init();
	
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
			
			case 'S':
				if (opts.cmd != VUN_NONE)
					cmd_help();
				else
					opts.cmd = VUN_SET;
				break;
			
			case 'G':
				if (opts.cmd != VUN_NONE)
					cmd_help();
				else
					opts.cmd = VUN_GET;
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			case 'n':
				opts.names = list_parse_hash(optarg, delim, kvdelim);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	switch (opts.cmd) {
		case VUN_GET:
			list_foreach(vp, i) {
				/* let's make a pointer to prevent unreadable code */
				list_node_t *vnode = vp->node+i;
				
				/* convert list data */
				uint64_t field  = *(uint64_t *)vnode->data;
				char *fieldname = vnode->key;
				
				vhi_name.field = field;
				
				/* syscall */
				if (vx_get_vhi_name(opts.xid, &vhi_name) == -1)
					PEXIT("Failed to get VHI field", EXIT_COMMAND);
				
				_lucid_printf("%s: %s\n", fieldname, vhi_name.name);
			}
			break;
		
		case VUN_SET: {
			if (opts.names == 0)
				goto out;
			
			list_link_t link = {
				.p = vp,
				.d = opts.names,
			};
			
			/* validate descending list */
			if (list_validate(&link) == -1)
				PEXIT("List validation failed", EXIT_USAGE);
			
			/* we kinda misuse the list parser here:
			 * the pristine list is used to decide where to put the data
			 * of the descending list with a matching key
			 */
			list_foreach(link.d, i) {
				/* let's make a pointer to prevent unreadable code */
				list_node_t *dnode = (link.d)->node+i;
				
				/* find vhi field to given key from descending list */
				list_node_t *pnode = list_search(link.p, dnode->key);
				
				/* convert list data */
				uint64_t field = *(uint64_t *)pnode->data;
				char *name     = (char *)dnode->data;
				
				vhi_name.field = field;
				strncpy(vhi_name.name, name, VHILEN-2);
				vhi_name.name[VHILEN-1] = '\0';
				
				/* syscall */
				if (vx_set_vhi_name(opts.xid, &vhi_name) == -1)
					PEXIT("Failed to set VHI field", EXIT_COMMAND);
			}
			break;
		}
		default:
			cmd_help();
	}
		
out:
	exit(EXIT_SUCCESS);
}
