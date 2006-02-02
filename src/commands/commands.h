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

#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>

#include "printf.h"

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

extern struct option const common_long_opts[];

#define COMMON_LONG_OPTS \
	{ "help",    no_argument, NULL, 'h' }, \
	{ "quiet",   no_argument, NULL, 'q' }, \
	{ "version", no_argument, NULL, 'V' }, \
	{ NULL,      no_argument, NULL, 0x0 }

#define COMMON_GETOPT_CASES(command) \
	case 'q': dup2(open("/dev/null", O_WRONLY), STDERR_FILENO); break; \
	case 'V': VERSIONINFO(command ## _rcsid); break; \
	case 'h': command ## _usage(EXIT_SUCCESS); break; \
	default: command ## _usage(EXIT_FAILURE); break;

#define COMMON_OPTS_HELP \
	"  -h,--help       View this help message\n" \
	"  -q,--quiet      Suppres all warnings and errors\n" \
	"  -V,--version    View version information\n"

#define GETOPT_LONG(C, c) \
	getopt_long(argc, argv, "+" C ## _SHORT_OPTS, c ## _long_opts, NULL)

/* command prototypes */
#include "context.h"

/* command main function */
typedef int (*COMMAND)(int, char **);

/* command map */
struct command_t {
	const char *name;
	COMMAND func;
	const char *desc;
};

extern struct command_t commands[];

#endif
