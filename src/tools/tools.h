/***************************************************************************
 *   Copyright 2005 by the libvserver team                                 *
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

#define GLOBAL_CMDS "hVv"

#define GLOBAL_OPTS bool verbose

#define GLOBAL_OPTS_INIT .verbose = false

#define GLOBAL_CMDS_GETOPT \
case 'h': \
	cmd_help(); \
	break; \
\
case 'V': \
	CMD_VERSION(NAME, DESCR); \
	break; \
\
case 'v': \
	opts.verbose = true; \
	break; \

#define GLOBAL_HELP "    -h            Display this help message\n" \
	"    -V            Display vserver-utils version\n" \
	"    -v            Enable verbose output\n"

#define DEFAULT_GETOPT \
default: \
	_lucid_printf("Try '%s -h' for more information\n", argv[0]); \
	exit(EXIT_USAGE); \
	break; \


#define CMD_VERSION(name, desc) do { \
	_lucid_printf("%s -- %s\n", name, desc); \
	_lucid_printf("This program is part of %s\n\n", PACKAGE_STRING); \
	_lucid_printf("Copyright (c) 2005 The vserver-utils Team\n"); \
	_lucid_printf("This program is free software; you can redistribute it and/or\n"); \
	_lucid_printf("modify it under the terms of the GNU General Public License\n"); \
	exit(0); \
}	while(0)

/* exit, silent exit, perror exit
 *
 * exit code conventions:
 * 
 * 0 = OK
 * 1 = Wrong usage
 * 2 = A command failed
 * 3 = An opts specific function failed
 */
#define EXIT_USAGE   1
#define EXIT_COMMAND 2
#define EXIT_OPTS    3

#define VPRINTF(opts, fmt, ...) if ((opts)->verbose) _lucid_printf(fmt , __VA_ARGS__ )
#ifdef DEBUG
#define DEBUGF(fmt, ...) _lucid_printf("DEBUG: " fmt , __VA_ARGS__ )
#else
#define DEBUGF(fmt, ...)
#endif

#define EXIT(MSG,RC) { \
	_lucid_printf(MSG"; try '%s -h' for more information\n", argv[0]); \
	exit(RC); \
}

#define SEXIT(MSG,RC) { \
	_lucid_printf(MSG"\n"); \
	exit(RC); \
}

#define PEXIT(MSG,RC) { \
	perror(MSG); \
	exit(RC); \
}
