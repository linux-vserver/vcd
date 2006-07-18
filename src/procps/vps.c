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

#define PS_BIN "/bin/ps"

xid_t masterxid = 1;
int error_mode = 0, spaces=0;

char *tail_name (char *vname) 
{   
	char *pch;
	pch = strrchr(vname, '/') + 1;
	
	return strdup(pch);
}

void parse_line (char *line, int n) 
{
	char *pid_pos, *pargv[128], *dupl, xid_string[128], *name = NULL;
	int max, i = 0;
	pid_t pid = -1;
	xid_t xid;
	struct vx_vhi_name vhi_name;
	
	vhi_name.field = VHIN_CONTEXT;

	if (n == 0) {
		if ((pid_pos = strstr(line, "PID")) == 0) {
			fprintf(stderr, "vps: cannot find the PID column, writing ps output\n");
			fprintf(stderr, "%s", line);
			error_mode = 1;
			return;
		}
		max = pid_pos-line+1;

		for (i=0; i < max + 3; i++)
			fprintf(stdout, "%c", line[i]);		
		fprintf(stdout, " XID            %s", line+max+3);
		return;
	}
	dupl = strdup(line);

	argv_from_str(line, pargv, 128);
	for(i=0;pargv[i];i++) {
		if (isdigit(*pargv[i])) {
			pid = atoi(pargv[i]);
			break;
		}
	}

	if(pid < 0)
		return;

	if ((xid = vx_get_task_xid(pid)) == -1)
		return;

	if (xid == 0)
		name = "ALL";
	else if (xid == 1)
		name = "WATCH";
	else {
		if (vx_get_vhi_name(xid, &vhi_name) == -1)
			return;
		if (vhi_name.name[0] == '/')
			name = tail_name(vhi_name.name);
		else
			name = vhi_name.name;
	}

	snprintf(xid_string, sizeof(xid_string), "%d %s", xid, name);

	if ((pid_pos = strstr(dupl, pargv[i])) == 0)
		return;
	max = pid_pos-dupl+strlen(pargv[i])+1;

	for (i=0; i < max; i++)
		fprintf(stdout, "%c", dupl[i]);
	fprintf(stdout, " %-14s %s", xid_string, dupl+max);
	return;
}	

int main (int argc, char *argv[])
{
	FILE *res;
	int i=0;
	char c, buffer[1024];

	/* Root access is needed */
	if (getuid()) {
		fprintf(stderr, "root access is needed\n");
		exit(EXIT_FAILURE);
	}

	/* Migrate to watch server */
	if (vx_migrate(masterxid, NULL) == -1) {
		fprintf(stderr, "cannot migrate to watch server, xid = %d\n", masterxid);
		exit(EXIT_FAILURE);
	}

	snprintf(buffer, sizeof(buffer) - 1, "%s ", PS_BIN);
	for(i=1;argv[i];i++)
		strncat(buffer, argv[i], sizeof(buffer)-strlen(buffer));

	i=0;
	res = popen(buffer, "r");
	while(!feof(res)) {
		memset(buffer, 0, sizeof(buffer));
		fgets(buffer, sizeof(buffer), res);
		if (error_mode)
			fprintf(stderr, "%s", buffer);
		else
			parse_line(buffer, i);
		i++;
	}
	pclose(res);

	exit(EXIT_SUCCESS);
}
