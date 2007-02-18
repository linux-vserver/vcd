// Copyright 2007 Benedikt Böhm <hollow@gentoo.org>
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

#ifndef _VGC_CMD_H
#define _VGC_CMD_H

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

typedef void (*CMD)(xmlrpc_env *env, int argc, char **argv);

typedef struct {
	char *name;
	CMD func;
} cmd_t;

extern cmd_t CMDS[];

#define return_if_fault(ENV) do { \
	if (ENV->fault_occurred) return; \
} while(0)

#define SIGNATURE(S) "({s:s,s:s}" S ")", "username", user, "password", pass

extern char *uri;
extern char *user;
extern char *pass;
extern char *name;

void usage(int rc);

void cmd_groupadd (xmlrpc_env *env, int argc, char **argv);
void cmd_groupdel (xmlrpc_env *env, int argc, char **argv);
void cmd_grouplist(xmlrpc_env *env, int argc, char **argv);

#endif
