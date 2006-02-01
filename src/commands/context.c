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

#include "context.h"

/* getopt settings */
#define CONTEXT_SHORT_OPTS "CMGS" COMMON_SHORT_OPTS

static struct option const context_long_opts[] = {
	{ "create",  1, NULL, 'C' },
	{ "migrate", 1, NULL, 'M' },
	{ "get",     2, NULL, 'G' },
	{ "set",     1, NULL, 'S' },
	COMMON_LONG_OPTS
};

static const char context_rcsid[] = "$Id$";

static inline
int context_usage(int rc)
{
	vu_printf("Usage: context [<opts>] <command> [-- <program> <args>*]\n"
	          "\n"
	          "Available commands:\n"
	          "    -C,--create <xid> [flags=<list>]\n"
	          "    -M,--migrate <xid>\n"
	          "    -G,--get [<xid>]\n"
	          "    -S,--set <xid> bcaps|ccaps|flags=<list>*\n"
	          "\n"
	          "    <xid>         Context ID\n"
	          "    <list>        List of caps/flags ([~]<flag>,[~]<flag>,...)\n"
	          "\n"
	          "Available options:\n"
	          COMMON_OPTS_HELP
	          "\n");
	exit(rc);
}

int context_create(char *xid, char *list)
{
	/* syscall data */
	struct vx_create_flags flags = {
		.flags = 0,
	};
	
	if (list == NULL)
		goto create;
	
	/* dummy */
	uint64_t mask = 0;
	
	/* parse flags */
	list64_parse(list, cflags_list, &flags.flags, &mask);
	
create:
	if (vx_create(atoi(xid), &flags) == -1)
		errfp("%d", atoi(xid));
	
	return 0;
}

int context_migrate(char *xid)
{
	if (vx_migrate(atoi(xid)) == -1)
		errfp("%d", atoi(xid));
	
	return 0;
}

int context_get(char *xid)
{
	return 12;
}

int context_set(char *xid, char *flags)
{
	return 13;
}

int context_main(int argc, char **argv)
{
	int c, rc = 0;
	
	while (1) {
		c = GETOPT_LONG(CONTEXT, context);
		if (c == -1) break;
		
		switch (c) {
			case 'C':
				if (argc < 3)
					context_usage(EXIT_FAILURE);
				
				if (argc < 4)
					rc = context_create(argv[2], NULL);
				else
					rc = context_create(argv[2], argv[3]);
				
				if (argc > 5)
					execvp(argv[5], argv+5);
				
				return rc;
			
			case 'M':
				if (argc < 3)
					context_usage(EXIT_FAILURE);
				
				rc = context_migrate(argv[2]);
				
				if (argc > 4)
					execvp(argv[4], argv+4);
				
				return rc;
			
			case 'G':
				if (argc < 3)
					return context_get("0");
				else
					return context_get(argv[2]);
				break;
			
			case 'S':
				if (argc < 3)
					context_usage(EXIT_FAILURE);
				
				int i;
				
				for (i = 3; i < argc; i++)
					context_set(argv[2], argv[i]);
				
				return EXIT_SUCCESS;
				break;
			
			COMMON_GETOPT_CASES(context)
		}
	}
	
	context_usage(EXIT_SUCCESS);
}
