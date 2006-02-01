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

#ifndef _COMMANDS_H_
#define _COMMANDS_H_

/* command prototypes */
int context_main(int, char **);

/* command main function */
typedef int (*COMMAND)(int, char **);

/* command map */
struct command_t {
	const char *name;
	COMMAND func;
	const char *desc;
} commands[] = {
	/* keep sorted */
	{ "context",   context_main,   "context commands" },
	
	/* end of list */
	{ NULL, NULL, NULL }
};

/* common macros */
#define VERSIONINFO(rcsid) do { \
	vu_printf("%s\n", rcsid); \
	vu_printf("This program is part of %s\n\n", PACKAGE_STRING); \
	vu_printf("Copyright (c) 2005-2006 The vserver-utils Team\n"); \
	vu_printf("This program is free software; you can redistribute it and/or\n"); \
	vu_printf("modify it under the terms of the GNU General Public License\n"); \
	exit(0); \
} while(0)

#define COMMON_SHORT_OPTS "hqV"

#define COMMON_LONG_OPTS \
	{ "help",    no_argument, NULL, 'h' }, \
	{ "quiet",   no_argument, NULL, 'q' }, \
	{ "version", no_argument, NULL, 'V' }, \
	{ NULL,      no_argument, NULL, 0x0 }

#define COMMON_GETOPT_CASES(applet) \
	case 'q': dup2(open("/dev/null", O_WRONLY), STDERR_FILENO); break; \
	case 'V': VERSIONINFO(applet ## _rcsid); break; \
	case 'h': applet ## _usage(EXIT_SUCCESS); break; \
	default: applet ## _usage(EXIT_FAILURE); break;

#define COMMON_OPTS_HELP \
	"    -h,--help     View this help message\n" \
	"    -q,--quiet    Suppres all warnings and errors\n" \
	"    -V,--version  View version information\n"

#define GETOPT_LONG(A, a) \
	getopt_long(argc, argv, "+" A ## _SHORT_OPTS, a ## _long_opts, NULL)

#endif
