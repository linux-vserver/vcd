/***************************************************************************
 *   Copyright 2005 by the vserver-utils team                              *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <vserver.h>

#include <linux/vserver/context.h>
#include <linux/vserver/network.h>

#include "tools.h"

#define NAME  "vps"
#define DESCR "Show details about running processes"

#define SHORT_OPTS "NXx:n:"

#define HUNK_SIZE 0x4000

struct options {
	bool shownid;
	bool showxid;
	nid_t nid;
	xid_t xid;
};

struct psline {
	char* lstart;
	char* pidend;
	pid_t pid;
	xid_t xid;
	nid_t nid;
};

static inline
void cmd_help()
{
	printf("Usage: %s <opts>* -- <ps args>*\n"
	       "\n"
	       "Available options:\n"
	       "    -n <nid>      Network Context ID\n"
	       "    -N            Display network context ID\n"
	       "    -x <xid>      Context ID\n"
	       "    -X            Display context ID\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

static
char *read_output(int fd, size_t *total_len)
{
	size_t len = 2*HUNK_SIZE;
	char *buf = malloc(len+1);
	size_t offset = 0;
	
	for (;;) {
		size_t l;
		
		while (offset >= len) {
			len += HUNK_SIZE;
			buf  = realloc(buf, len+1);
		}
		
		l = read(fd, buf+offset, len - offset);
		
		if (l == 0)
			break;
		
		offset += l;
	}
	
	buf[offset] = '\0';
	
	if (total_len)
		*total_len = offset;
	
	return buf;
}

static
pid_t extract_pid(char *lstart, char *pid_end)
{
	while (pid_end > lstart && pid_end[-1] != ' ')
		pid_end--;
	
	return atoi(pid_end);
}

static
void process_output(char *data, size_t len, bool showxid, bool shownid, nid_t nid)
{
	int lmaxcnt = 100;
	int lcnt = 1;
	struct psline* pslines = malloc(lmaxcnt * sizeof(struct psline));
	size_t pid_end, xidwidth, nidwidth;
	char * eol_pos = strchr(data, '\n');
	char * pos;

	if (eol_pos == 0)
		eol_pos  = data + len;
	else
		*eol_pos = '\0';

	pos = strstr(data, "PID");
	if (pos == 0) {
		// We don't have output we we know position of pid,
		// possibly help message
		// Just forward it
		printf("vps: PID column not found, dumping ps's output.\n\n");
		write(1, data, len);
		return;
	}

	pid_end = pos-data + 4;
	pslines[0].lstart = data;
	pslines[0].pidend = data + pid_end;
	pslines[0].pid = 0;
	pslines[0].xid = 0;
	pslines[0].nid = 0;

	// read the PIDs...
	len -= eol_pos-data;
	data = eol_pos+1;
	while (len >1) {
		--len;
		eol_pos = strchr(data, '\n');

		if (eol_pos==0) eol_pos  = data + len;

		pslines[lcnt].lstart = data;
		pslines[lcnt].pidend = data + pid_end;
		pslines[lcnt].pid    = extract_pid(data, data+pid_end);
		if (pslines[0].pid < pslines[lcnt].pid)
			pslines[0].pid = pslines[lcnt].pid; // Remember max PID
		pslines[lcnt].xid    = vx_get_task_xid(pslines[lcnt].pid);
		if (pslines[0].xid < pslines[lcnt].xid)
			pslines[0].xid = pslines[lcnt].xid; // Remember max XID
		pslines[lcnt].nid    = nx_get_task_nid(pslines[lcnt].pid);
		if (pslines[0].nid < pslines[lcnt].nid)
			pslines[0].nid = pslines[lcnt].nid; // Remember max NID

		if (nid <= 1 || nid == pslines[lcnt].nid) {
			// Only consider if we don't filter on nid or nid matches
			lcnt++;
			if (lcnt >= lmaxcnt) {
				// Buffer getting small, enlarge
				struct psline*tmp = realloc(pslines, (lmaxcnt+100)*sizeof(struct psline));
				if (tmp) {
					pslines = tmp;
					lmaxcnt += 100;
				} else break; // out of memory, just print what we already have
			}
		}
		len -= eol_pos-data;
		data = eol_pos+1;
	}

	// Now print the whole thing
	xidwidth = snprintf(NULL, 0, "%d ", pslines[0].xid);
	if (xidwidth < 4) xidwidth = 4; // Make sure the title fits
	nidwidth = snprintf(NULL, 0, "%d ", pslines[0].nid);
	if (nidwidth < 4) nidwidth = 4; // Make sure the title fits

	write(1, pslines[0].lstart, pid_end);
	if (showxid) {
		write(1, "                    ", xidwidth-4);
		write(1, "XID ", 4);
	}
	if (shownid) {
		write(1, "                    ", nidwidth-4);
		write(1, "NID ", 4);
	}
	eol_pos = strchr(pslines[0].pidend, '\n');
	write(1, pslines[0].pidend, eol_pos-pslines[0].pidend);
	write(1, "\n", 1);

	for (lmaxcnt = 1; lmaxcnt < lcnt; lmaxcnt++) {
		char tmp[20];
		int n = 0;
		write(1, pslines[lmaxcnt].lstart, pid_end);
		if (showxid) {
			n = snprintf(tmp, sizeof(tmp), "%d ", pslines[lmaxcnt].xid);
			write(1, "                    ", xidwidth-n);
			write(1, tmp, n);
		}
		if (shownid) {
			n = snprintf(tmp, sizeof(tmp), "%d ", pslines[lmaxcnt].nid);
			write(1, "                    ", nidwidth-n);
			write(1, tmp, n);
		}
		eol_pos = strchr(pslines[lmaxcnt].pidend, '\n');
		write(1, pslines[lmaxcnt].pidend, eol_pos-pslines[lmaxcnt].pidend);
		write(1, "\n", 1);
	}
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct options opts = {
		.shownid = false,
		.showxid = false,
		.nid    = 0,
		.xid    = 1,
	};
	
	int c;
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'X':
				opts.showxid = true;
				break;
			
			case 'N':
				opts.shownid = true;
				break;
			
			case 'n':
				opts.nid = (nid_t) atoi(optarg);
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	/* show help message */
	if (argc == 1)
		cmd_help();
	
	if (opts.xid == 1)
		opts.showxid = true;
	if (opts.nid <= 1)
		opts.shownid = true;
	
	if (vx_migrate(opts.xid) == -1)
		PEXIT("Failed to migrate to context", EXIT_COMMAND);
	
	/* create pipe for ps */
	int p[2];
	pipe(p);
	
	/* fork ps */
	pid_t pid;
	pid = fork();
	
	if (pid == 0) {
		int fd = open("/dev/null", O_RDONLY, 0);
		
		dup2(fd,   0);
		dup2(p[1], 1);
		
		close(p[0]);
		close(p[1]);
		close(fd);
		
		argv[optind-1] = "ps";
		
		if (execv("/bin/ps", argv+optind-1) == -1)
			PEXIT("Failed to start ps", EXIT_COMMAND);
	}
	
	/* get output from ps */
	char *data;
	size_t len;
	
	close(p[1]);
	data = read_output(p[0], &len);
	close(p[0]);
	
	/* parse output */
	process_output(data, len, opts.showxid, opts.shownid, opts.nid);
	
	waitpid(pid, &c, 0);
	
	return WIFEXITED(c) ? WEXITSTATUS(c) : 1;
}
