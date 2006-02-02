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
#include "config.h"
#endif

#include "msg.h"
#include "printf.h"

#include "commands.h"
#include "context.h"

static const char context_rcsid[] = "$Id$";

static
int context_usage(int rc)
{
	vu_printf("Usage: context [<opts>] <command> [-- <program> <args>*]\n"
	          "\n"
	          "Available commands:\n"
	          "    create <xid> [flags=<list>]\n"
	          "    migrate <xid>\n"
	          "    get bcaps|ccaps|flags [<xid>]\n"
	          "    set bcaps|ccaps|flags=<list>* <xid>\n"
	          "\n"
	          "    <xid>         Context ID\n"
	          "    <list>        List of caps/flags ([~]<flag>,[~]<flag>,...)\n"
	          "\n"
	          "Available options:\n"
	          COMMON_OPTS_HELP
	          "\n");
	exit(rc);
}

int context_create(char *id, char *list)
{
	xid_t xid = 0;
	
	/* syscall data */
	struct vx_create_flags flags = {
		.flags = 0,
	};
	
	if (list == NULL)
		goto create;
	
	/* dummy */
	uint64_t mask = 0;
	
	/* parse flags */
	list64_parse(list, cflags_list, &flags.flags, &mask, '~', ',');
	
create:
	xid = (xid_t) atoi(id);
	
	if (vx_create(xid, &flags) == -1)
		errfp("%d", xid);
	
	return 0;
}

int context_migrate(char *id)
{
	xid_t xid = (xid_t) atoi(id);
	
	if (vx_get_info(xid, NULL) == -1) {
		if (vx_create(xid, NULL) == -1)
			errfp("%d", xid);
	} else {
		if (vx_migrate(xid) == -1)
			errfp("%d", xid);
	}
	
	return 0;
}

int context_get(char *id, char *type)
{
	int i;
	xid_t xid;
	
	/* init syscall data */
	struct vx_flags flags = {
		.flags = ~(0ULL),
		.mask  = ~(0ULL),
	};
	
	struct vx_caps caps = {
		.bcaps = ~(0ULL),
		.bmask = ~(0ULL),
		.ccaps = ~(0ULL),
		.cmask = ~(0ULL),
	};
	
	if (id != NULL) {
		xid = (xid_t) atoi(id);
		
		if (vx_get_caps(xid, &caps) == -1)
			errfp("%d", id);
		
		if (vx_get_flags(xid, &flags) == -1)
			errfp("%d", id);
	}
	
	char *buf = NULL;
	
	if (strcmp(type, "bcaps") == 0)
		buf = list64_tostr(caps.bcaps, bcaps_list, '\n');
	
	else if (strcmp(type, "ccaps") == 0)
		buf = list64_tostr(caps.ccaps, ccaps_list, '\n');
	
	else if (strcmp(type, "cflags") == 0)
		buf = list64_tostr(flags.flags, cflags_list, '\n');
	
	else
		errf("Unknown type: %s\n", type);
	
	vu_printf("%s", buf);
	
	free(buf);
	return 0;
}

int context_set(char *id, char *type, char *list)
{
	int i;
	xid_t xid;
	
	/* init syscall data */
	struct vx_flags flags = {
		.flags = (0ULL),
		.mask  = (0ULL),
	};
	
	struct vx_caps caps = {
		.bcaps = ~(0ULL),
		.bmask = ~(0ULL),
		.ccaps =  (0ULL),
		.cmask =  (0ULL),
	};
	
	xid = (xid_t) atoi(id);
	
	if (strcmp(type, "bcaps") == 0)
		list64_parse(list, bcaps_list, &caps.bcaps, &caps.bmask, '~', ',');
	
	else if (strcmp(type, "ccaps") == 0)
		list64_parse(list, ccaps_list, &caps.ccaps, &caps.cmask, '~', ',');
	
	else if (strcmp(type, "cflags") == 0)
		list64_parse(list, cflags_list, &flags.flags, &flags.mask, '~', ',');
	
	else
		errf("Unknown type: %s\n", type);
	
	if (vx_set_caps(xid, &caps) == -1)
		errfp("%d", id);
	
	if (vx_set_flags(xid, &flags) == -1)
		errfp("%d", id);
	
	return 0;
}

int context_main(int argc, char **argv)
{
	int c, rc = 0;
	
	while ((c = GETOPT_LONG(COMMON, common)) != -1) {
		switch (c) {
			COMMON_GETOPT_CASES(context)
		}
	}
	
	int idx = optind;
	
	if (optind < argc) {
		/* create */
		if (strcmp(argv[idx], "create") == 0) {
			idx++;
			
			if (idx < argc) {
				switch(argc - idx) {
					case 1:
						rc = context_create(argv[idx], NULL);
						break;
					
					case 2:
						rc = context_create(argv[idx], argv[idx+1]);
						idx++;
						break;
					
					default:
						errf("%s", "Invalid number of arguments");
						break;
				}
			}
			
			else
				errf("%s", "No xid given");
		}
		
		/* migrate */
		else if (strcmp(argv[idx], "migrate") == 0) {
			idx++;
			
			if (idx < argc) {
				context_migrate(argv[idx]);
				
				idx++; /* skip xid */
				idx++; /* skip '--' */
				
				if (idx < argc)
					if (execvp(argv[idx], argv+idx) == -1)
						errfp("%s", "execvp");
			}
			
			else
				errf("%s", "No xid given");
		}
		
		/* get */
		else if (strcmp(argv[idx], "get") == 0) {
			idx++;
			
			if (idx < argc) {
				switch(argc - idx) {
					case 1:
						rc = context_get(NULL, argv[idx]);
						break;
					
					case 2:
						rc = context_get(argv[idx+1], argv[idx]);
						idx++;
						break;
					
					default:
						errf("%s", "Invalid number of arguments");
						context_usage(EXIT_FAILURE);
						break;
				}
			}
			
			else
				errf("%s", "No type given");
		}
		
		/* set */
		else if (strcmp(argv[idx], "set") == 0) {
			idx++;
			
			char *p;
			
			if (idx < argc) {
				switch(argc - idx) {
					case 2:
						p = strchr(argv[idx], '=');
						
						if (p == NULL)
							errf("%s", "Invalid flags list");
						
						memset(p++, '\0', 1);
						
						rc = context_set(argv[idx+1], argv[idx], p);
						idx++;
						break;
					
					default:
						errf("%s", "Invalid number of arguments");
						context_usage(EXIT_FAILURE);
						break;
				}
			}
			
			else
				errf("%s", "No type given");
		}
		
		else
			errf("Unknown command: %s\n", argv[idx]);
	}
	
	else
		errf("%s", "No command given");
	
	return rc;
}
