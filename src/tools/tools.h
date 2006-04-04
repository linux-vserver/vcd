// Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <getopt.h>

#include "lucid.h"

#define COMMON_LONG_OPTS \
	{ "help",    0, 0, 0x01 }, \
	{ "version", 0, 0, 0x02 },

#define COMMON_GETOPT_CASES \
	case 0x01: usage(EXIT_SUCCESS); break; \
	case 0x02: printf("%s\n", rcsid); exit(EXIT_SUCCESS); break;

#define DEFAULT_GETOPT_CASES default: usage(EXIT_FAILURE); break;

#define GETOPT(VAL) ((VAL = getopt_long_only(argc, argv, "", long_opts, NULL)) != -1)

/* msg.c */
#define INIT_ARGV0 argv0 = argv[0];

extern const char *argv0;

void warn  (const char *fmt, /*args*/ ...);
void pwarn (const char *fmt, /*args*/ ...);
void err   (const char *fmt, /*args*/ ...);
void perr  (const char *fmt, /*args*/ ...);

/* lists.c */
extern const flist64_t bcaps_list[];
extern const flist64_t ccaps_list[];
extern const flist64_t cflags_list[];
extern const flist32_t vhiname_list[];
extern const flist32_t iattr_list[];
extern const flist32_t rlimit_list[];
extern const flist64_t nflags_list[];
extern const flist32_t sched_list[];
