// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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

#ifndef _VCC_VCC_H
#define _VCC_VCC_H

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

typedef void (*CMD)(xmlrpc_env *env, int argc, char **argv);

typedef struct {
	char *name;
	CMD func;
} cmd_t;

extern cmd_t CMDS[];

extern char *uri;
extern char *user;
extern char *pass;

void do_vlogin(int argc, char **argv);

#define return_if_fault(ENV) do { \
	if (ENV->fault_occurred) return; \
} while(0)

#define client_call(M, S, ...) \
	xmlrpc_client_call(env, uri, M, "({s:s,s:s}" S ")", \
			"username", user, "password", pass, __VA_ARGS__)

#define client_call0(M) \
	xmlrpc_client_call(env, uri, M, "({s:s,s:s}s)", \
			"username", user, "password", pass, "")

#define COMMON_USAGE \
	"Available options:\n" \
	"   -c <path>     configuration file (default: " SYSCONFDIR "/vcc.conf)\n" \
	"   -h <host>     server hostname (default: localhost)\n" \
	"   -p <port>     server port     (default: 13386)\n" \
	"   -u <user>     server username (default: admin)\n"

#endif
