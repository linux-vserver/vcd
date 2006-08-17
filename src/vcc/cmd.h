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

#ifndef _VCC_CMD_H
#define _VCC_CMD_H

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

void do_vlogin(int argc, char **argv);

void usage(int rc);

void cmd_create (xmlrpc_env *env, int argc, char **argv);
void cmd_exec   (xmlrpc_env *env, int argc, char **argv);
void cmd_kill   (xmlrpc_env *env, int argc, char **argv);
void cmd_login  (xmlrpc_env *env, int argc, char **argv);
void cmd_reboot (xmlrpc_env *env, int argc, char **argv);
void cmd_remove (xmlrpc_env *env, int argc, char **argv);
void cmd_rename (xmlrpc_env *env, int argc, char **argv);
void cmd_start  (xmlrpc_env *env, int argc, char **argv);
void cmd_status (xmlrpc_env *env, int argc, char **argv);
void cmd_stop   (xmlrpc_env *env, int argc, char **argv);

#endif
