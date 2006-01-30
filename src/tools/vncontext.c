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
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <vserver.h>

#include <linux/vserver/network.h>

#include "printf.h"
#include "tools.h"

#define NAME  "vncontext"
#define DESCR "Network Context Manager"

#define SHORT_OPTS "ACIMRa:f:n:"

struct commands {
	bool add;
	bool create;
	bool info;
	bool migrate;
	bool remove;
};

struct options {
	char *addr;
	list_t *flags;
	nid_t nid;
};

static inline
void cmd_help()
{
 vu_printf("Usage: %s <command> <opts>* -- <program> <args>*\n"
	       "\n"
	       "Available commands:\n"
	       "    -A            Add adress to network context\n"
	       "    -C            Create a new network context\n"
	       "    -I            Get information about a network context\n"
	       "    -M            Migrate to an existing network context\n"
	       "    -R            Remove adress from network context\n"
	       "\n"
	       "Available options:\n"
	       "    -a <addr>     IP adress (a.b.c.d/e)\n"
	       "    -f <list>     Set flags described in <list>\n"
	       "    -n <nid>      Context ID\n"
	       "\n"
	       "Flag list format string:\n"
	       "    <list> = [~]<flag>,[~]<flag>,...\n"
	       "\n"
	       "    See 'vnflags -L' for available flags\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

static inline
int parse_addr(char *addr, uint32_t *ip, uint32_t *mask)
{
		struct in_addr ib;
		char *addr_ip, *addr_mask;
		
		*ip   = 0;
		*mask = 0;
		
		addr_ip   = strtok(addr, "/");
		addr_mask = strtok(NULL, "/");
		
		if (addr_ip == 0)
			return -1;
		
		if (inet_aton(addr_ip, &ib) == -1)
			return -1;
		
		*ip = ib.s_addr;
		
		if (addr_mask == 0) {
			/* default to /24 */
			*mask = ntohl(0xffffff00);
		} else {
			if (strchr(addr_mask, '.') == 0) {
				/* We have CIDR notation */
				int sz = atoi(addr_mask);
				
				for (*mask = 0; sz > 0; --sz) {
					*mask >>= 1;
					*mask  |= 0x80000000;
				}
				
				*mask = ntohl(*mask);
			} else {
				/* Standard netmask notation */
				if (inet_aton(addr_mask, &ib) == -1)
					return -1;
				
				*mask = ib.s_addr;
			}
		}
		return 0;
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct commands cmds = {
		.add      = false,
		.create   = false,
		.info     = false,
		.migrate  = false,
		.remove   = false,
	};
	
	struct options opts = {
		.addr  = 0,
		.flags = 0,
		.nid   = 0,
	};
	
	/* init syscall data */
	struct nx_create_flags create_flags = {
		.flags = 0,
	};
	
	struct nx_addr addr;
	struct nx_info info;
	
	/* init flags list */
	list_t *np = nflags_list_init();
	
	int c;
	const char delim = ','; // list delimiter
	const char clmod = '~'; // clear flag modifier
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'A':
				cmds.add = true;
				break;
			
			case 'C':
				cmds.create = true;
				break;
			
			case 'I':
				cmds.info = true;
				break;
			
			case 'M':
				cmds.migrate = true;
				break;
			
			case 'R':
				cmds.remove = true;
				break;
			
			case 'a':
				opts.addr = optarg;
				break;
			
			case 'f':
				opts.flags = list_parse_list(optarg, delim);
				break;
			
			case 'n':
				opts.nid = (nid_t) atoi(optarg);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	if (cmds.add) {
		if (opts.addr == 0)
			EXIT("No IP given", EXIT_USAGE);
		
		addr.type  = NXA_TYPE_IPV4;
		addr.count = 1;
		
		if (parse_addr(opts.addr, &addr.ip[0], &addr.mask[0]) == -1)
			EXIT("Invalid IP given", EXIT_USAGE);
		
		/* syscall */
		if (nx_add_addr(opts.nid, &addr) == -1)
			PEXIT("Failed to add network address", EXIT_COMMAND);
		
		goto out;
	}
	
	if (cmds.create) {
		if (opts.flags == 0)
			goto create;
		
		list_link_t link = {
			.p = np,
			.d = opts.flags,
		};
		
		/* validate descending list */
		if (list_validate_flag(&link, clmod) == -1)
			PEXIT("List validation failed", EXIT_USAGE);
		
		/* vx_create_flags has no mask member
		 * so we create a dumb one */
		uint64_t mask = 0;
		
		/* convert given descending list to flags using the pristine copy */
		list_list2flags(&link, clmod, &create_flags.flags, &mask);
		
create:
		/* syscall */
		if (nx_create(opts.nid, &create_flags) == -1)
			PEXIT("Failed to create network context", EXIT_COMMAND);
		
		goto load;
	}
	
	if (cmds.migrate) {
		/* syscall */
		if (nx_migrate(opts.nid) == -1)
			PEXIT("Failed to migrate to network context", EXIT_COMMAND);
		
		goto load;
	}
	
	if (cmds.info) {
		/* syscall */
		if (nx_get_info(opts.nid, &info) == -1)
			PEXIT("Failed to get network context information", EXIT_COMMAND);
		
	 vu_printf("Network context ID: %d\n", info.nid);
		
		goto out;
	}
	
	if (cmds.remove) {
		if (opts.addr == 0)
			EXIT("No IP given", EXIT_USAGE);
		
		addr.type  = NXA_TYPE_IPV4;
		addr.count = 1;
		
		if (parse_addr(opts.addr, &addr.ip[0], &addr.mask[0]) == -1)
			EXIT("Invalid IP given", EXIT_USAGE);
		
		/* syscall */
		if (nx_rem_addr(opts.nid, &addr) == -1)
			PEXIT("Failed to remove network address", EXIT_COMMAND);
		
		goto out;
	}
	
	cmd_help();
	
load:
	if (argc > optind)
		execvp(argv[optind], argv+optind);
	
out:
	exit(EXIT_SUCCESS);
}
