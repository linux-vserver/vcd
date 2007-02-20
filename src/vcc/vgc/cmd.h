// Copyright 2007 Luca Longinotti <chtekk@gentoo.org>
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

void usage(int rc);

void cmd_groupadd     (xmlrpc_env *env, int argc, char **argv);
void cmd_grouplist    (xmlrpc_env *env, int argc, char **argv);
void cmd_groupremove  (xmlrpc_env *env, int argc, char **argv);
void cmd_vserveradd   (xmlrpc_env *env, int argc, char **argv);
void cmd_vserverlist  (xmlrpc_env *env, int argc, char **argv);
void cmd_vserverremove(xmlrpc_env *env, int argc, char **argv);
void cmd_wrapperstart (xmlrpc_env *env, int argc, char **argv);
void cmd_wrapperstop  (xmlrpc_env *env, int argc, char **argv);

#endif