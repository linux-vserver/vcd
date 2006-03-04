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
#include <string.h>
#include <sys/wait.h>
#include <vserver.h>

#include "printf.h"
#include "tools.h"

#define NAME  "vps"
#define DESCR "Show details about running processes"

#define SHORT_OPTS "NXx:n:"

#define HUNK_SIZE 0x4000

struct options {
	GLOBAL_OPTS;
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
	vu_printf("Usage: %s <opts>* -- <ps args>*\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
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
	if (pid_end > lstart) pid_end--;
	
	while (pid_end > lstart && pid_end[-1] != ' ')
		pid_end--;
	
	return atoi(pid_end);
}

static
void process_output(char *data, size_t len, struct options *opts)
{
	char *eol = strchr(data, '\n');
	
	/* find end of line */
	if (eol == 0)
		eol = data + len;
	else
		*eol = '\0';
	
	char *pid_start;
	
	/* find PID column */
	pid_start = strstr(data, "PID");
	
	if (pid_start == 0) {
		/* we don't have output with PID column; possibly help message
		** just forward it */
		vu_printf("vps: PID column not found, dumping ps's output.\n\n");
		write(1, data, len);
		return;
	}
	
	int lmaxcnt = 100;
	struct psline* pslines = malloc(lmaxcnt * sizeof(struct psline));
	
	size_t pid_end;
	
	/* header */
	pid_end = pid_start - data + 4;
	pslines[0].lstart = data;
	pslines[0].pidend = data + pid_end;
	pslines[0].pid = 0;
	pslines[0].xid = 0;
	pslines[0].nid = 0;
	
	/* skip header */
	len -= eol - data;
	data = eol + 1;
	
	int lcnt = 1;
	
	/* read lines from ps */
	while (len > 1) {
		--len;
		
		eol = strchr(data, '\n');
		
		if (eol == 0)
			eol = data + len;
		
		pslines[lcnt].lstart = data;
		pslines[lcnt].pidend = data + pid_end;
		pslines[lcnt].pid    = extract_pid(data, data+pid_end);
		
		/* special case uglyness for guests init process */
		if (opts->xid > 1 && pslines[lcnt].pid == 1) {
			struct vx_info info;
			vx_get_info(opts->xid, &info);
			
			pslines[lcnt].xid    = vx_get_task_xid(info.initpid);
			pslines[lcnt].nid    = nx_get_task_nid(info.initpid);
		} else {
			pslines[lcnt].xid    = vx_get_task_xid(pslines[lcnt].pid);
			pslines[lcnt].nid    = nx_get_task_nid(pslines[lcnt].pid);
		}
		
		/* remember highest values */
		if (pslines[0].pid < pslines[lcnt].pid)
			pslines[0].pid = pslines[lcnt].pid;
		
		if (pslines[0].xid < pslines[lcnt].xid)
			pslines[0].xid = pslines[lcnt].xid;
		
		if (pslines[0].nid < pslines[lcnt].nid)
			pslines[0].nid = pslines[lcnt].nid;
		
		/* skip entries with non-matching nid */
		if (opts->nid <= 1 || opts->nid == pslines[lcnt].nid) {
			lcnt++;
			
			/* line buffer runs out of space */
			if (lcnt >= lmaxcnt) {
				struct psline *tmp = realloc(pslines, (lmaxcnt+100)*sizeof(struct psline));
				
				if (tmp) {
					pslines  = tmp;
					lmaxcnt += 100;
				}
				
				/* out of memory */
				else
					break;
			}
		}
		
		len -= eol - data;
		data = eol + 1;
	}
	
	size_t xidwidth, nidwidth;
	
	/* get width of xid/nid columns
	** max xid/nid is 65535; i.e. the column does never grow > 6 */
	xidwidth = vu_snprintf(NULL, 0, "%d ", pslines[0].xid);
	nidwidth = vu_snprintf(NULL, 0, "%d ", pslines[0].nid);
	
	/* just in case the header wouldn't fit */
	if (xidwidth < 4)
		xidwidth = 4;
	
	if (nidwidth < 4)
		nidwidth = 4;
	
	/* print the header */
	write(1, pslines[0].lstart, pid_end);
	
	if (opts->showxid) {
		write(1, "      ", xidwidth - 4);
		write(1, "XID ", 4);
	}
	
	if (opts->shownid) {
		write(1, "      ", nidwidth - 4);
		write(1, "NID ", 4);
	}
	
	write(1, pslines[0].pidend, strlen(pslines[0].pidend));
	write(1, "\n", 1);
	
	int i;
	
	/* write ps lines */
	for (i = 1; i < lcnt; i++) {
		char tmp[6];
		int n = 0;
		
		write(1, pslines[i].lstart, pid_end);
		
		if (opts->showxid) {
			n = vu_snprintf(tmp, sizeof(tmp), "%d ", pslines[i].xid);
			write(1, "      ", xidwidth - n);
			write(1, tmp, n);
		}
		
		if (opts->shownid) {
			n = vu_snprintf(tmp, sizeof(tmp), "%d ", pslines[i].nid);
			write(1, "      ", nidwidth - n);
			write(1, tmp, n);
		}
		
		eol = strchr(pslines[i].pidend, '\n');
		
		write(1, pslines[i].pidend, eol - pslines[i].pidend);
		write(1, "\n", 1);
	}
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.shownid = false,
		.showxid = false,
		.nid    = 0,
		.xid    = 1,
	};
	
	int c;

	DEBUGF("%s: starting ...\n", NAME);

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
	
	/* create pipe for ps */
	int p[2];
	pipe(p);
	
	/* fork ps */
	pid_t pid;
	pid = fork();
	
	if (pid == 0) {
		// TODO: chnamespace
		//if (vx_enter_namespace(opts.xid) == -1)
		//	PEXIT("Failed to enter namespace", EXIT_COMMAND);
		
		if (vx_migrate(opts.xid, NULL) == -1)
			PEXIT("Failed to migrate to context", EXIT_COMMAND);
		
		int fd = open("/dev/null", O_RDONLY, 0);
		
		dup2(fd,   0);
		dup2(p[1], 1);
		
		close(p[0]);
		close(p[1]);
		close(fd);
		
		argv[optind-1] = "ps";
		
		// TODO: chroot to context's root (for ps to map uid <-> user correctly) -- if one exists
		if (execv("/bin/ps", argv+optind-1) == -1)
			PEXIT("Failed to start ps", EXIT_COMMAND);
	} else if (pid == -1)
		PEXIT("Failed to clone process", EXIT_COMMAND);
	
	/* get output from ps */
	char *data;
	size_t len;
	
	close(p[1]);
	data = read_output(p[0], &len);
	close(p[0]);
	
	/* parse output */
	process_output(data, len, &opts);
	
	waitpid(pid, &c, 0);
	
	return WIFEXITED(c) ? WEXITSTATUS(c) : 1;
}
