// Copyright 2006 Remo Lemma <coloss7@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <vserver.h>

#include "lucid.h"

#include "../wrapper.h"

#define PS_BIN "/bin/ps"

int error_mode = 0;

static
char *tail_name(char *vname)
{
	char *pch;
	pch = strrchr(vname, '/') + 1;
	return strdup(pch);
}

static
void parse_line(char *line, int n)
{
	char *pid_pos, *name = NULL;
	static int pid_start;
	pid_t pid;
	xid_t xid;
	struct vx_vhi_name vhi_name;
	
	if (n == 0) {
		if ((pid_pos = strstr(line, "  PID")) == 0) {
			dprintf(STDERR_FILENO, "vps: PID column not found, writing ps output\n");
			printf(line);
			error_mode = 1;
			return;
		}
		
		pid_start = pid_pos - line;
		
		printf("  XID NAME     %s\n", line);
		return;
	}
	
	else {
		pid = atoi(line + pid_start);
		
		if (pid < 0)
			return;
		
		if ((xid = vx_get_task_xid(pid)) == -1)
			return;
		
		if (xid == 0)
			name = "ADMIN";
		
		else if (xid == 1)
			name = "WATCH";
		
		else {
			vhi_name.field = VHIN_CONTEXT;
			
			if (vx_get_vhi_name(xid, &vhi_name) == -1)
				return;
			
			if (vhi_name.name[0] == '/')
				name = tail_name(vhi_name.name);
			
			else
				name = vhi_name.name;
		}
		
		printf("%5d %-8.8s %s\n", xid, name, line);
	}
}

int main(int argc, char **argv)
{
	int p[2], fd, i, status, len;
	pid_t pid;
	char *line;
	
	/* root access is needed */
	if (getuid())
		die("root access is needed");
	
	argv[0] = PS_BIN;
	
	pipe(p);
	
	switch ((pid = fork())) {
	case -1:
		pdie("fork");
	
	case 0:
		fd = open_read("/dev/null");
		
		dup2(fd,   0);
		dup2(p[1], 1);
		
		close(p[0]);
		close(p[1]);
		close(fd);
		
		if (vx_migrate(1, NULL) == -1)
			pdie("vx_migrate");
		
		if (execvp(argv[0], argv) == -1)
			pdie("execvp");
	
	default:
		close(p[1]);
		
		for (i = 0; ; i++) {
			if ((len = io_read_eol(p[0], &line)) == -1)
				pdie("io_read_eol");
			
			if (!len)
				break;
			
			if (error_mode)
				printf("%s\n", line);
			else
				parse_line(line, i);
		}
		
		close(p[0]);
		
		if (waitpid(pid, &status, 0) == -1)
			pdie("waitpid");
	}
	
	exit(EXIT_SUCCESS);
}