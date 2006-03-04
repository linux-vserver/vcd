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

#ifndef _VC_COMMANDS_H
#define _VC_COMMANDS_H

/* command types */
typedef void (*CMDM)(int, char **);
typedef void (*CMDH)(int);

typedef struct {
	char *name;
	CMDM main;
	CMDH help;
	int interactive;
} vc_cmd_t;

int do_command(int argc, char **argv, char **data);

/* main prototypes */
void start_main(int argc, char **argv);
void login_main(int argc, char **argv);

/* usage prototypes */
void start_help(int rc);
void login_help(int rc);

/* command map */
extern vc_cmd_t CMDS[];

#define SIGSEGV_MSG(CMD) do { \
	vc_warn("Segmentation Fault - you probably found a bug.\n" \
	        "\n" \
	        "You can help fixing it by following these instructions:\n" \
	        " * install the GNU Debugger (http://www.gnu.org/software/gdb)\n" \
	        " * compile %s with CFLAGS=\"-g -ggdb3\"\n" \
	        " * enter gdb with \"gdb --args %s %s <your args>\"\n" \
	        " * type \"run\" and wait for SIGSEGV to occur\n" \
	        " * type \"bt full\" to get a backtrace\n", \
	        " * send a bug report with the full output of gdb attached\n" \
	        "   to %s\n" \
	        "\n" \
	        "Thanks for your help!\n", \
	        PACKAGE_NAME, \
	        vc_argv0, \
	        #CMD, \
	        PACKAGE_BUGREPORT); \
} while(0)

#endif
