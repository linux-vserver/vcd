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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/file.h>
#include <string.h>

#include <lucid/printf.h>
#include "tools.h"

#define NAME  "lockfile"
#define DESCR "Lock a File"

#define SHORT_OPTS "l:s:t:"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct options {
	GLOBAL_OPTS;
	char *lockfile;
	char *syncpipe;
	int timeout;
};

/* init program data */
struct options opts = {
	GLOBAL_OPTS_INIT,
	.lockfile = NULL,
	.syncpipe = NULL,
	.timeout  = 300,
};

static inline
void cmd_help()
{
	_lucid_printf("Usage: %s <opts>*\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -l <file>     Lock file\n"
	       "    -s <fifo>     Sync pipe\n"
	       "    -t <sec>      Timeout\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

static
void signal_handler(int sig)
{
	switch(sig) {
		case SIGALRM:
			signal(SIGALRM, signal_handler);
			break;
		
		case SIGHUP:
			_exit(0);
			break;
		
		default:
			break;
	}
}

int main(int argc, char *argv[])
{
	int c;
	
	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'l':
				opts.lockfile = optarg;
				break;
			
			case 's':
				opts.syncpipe = optarg;
				break;
			
			case 't':
				opts.timeout = atoi(optarg);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	/* check command line */
	if (opts.lockfile == NULL || *opts.lockfile == '\0')
		EXIT("No lockfile specified", EXIT_USAGE);
	
	if (opts.syncpipe == NULL || *opts.syncpipe == '\0')
		EXIT("No syncpipe specified", EXIT_USAGE);
	
	pid_t ppid = getppid();
	time_t end = time(0) + opts.timeout;
	
	/* start locking */
	int syncfd, lockfd;
	
	if ((syncfd = open(opts.syncpipe, O_WRONLY)) == -1)
		perror("Failed to open syncpipe");
	
	else if ((lockfd = open(opts.lockfile, O_CREAT|O_RDWR|O_NOFOLLOW|O_NONBLOCK, 0644)) == -1)
		perror("Failed to open lockfile");
	
	else if (unlink(opts.syncpipe) == -1)
		perror("Failed ot unlink syncpipe");
	
	else if (siginterrupt(SIGALRM, 1) == -1)
		perror("Failed to set interrupt signal");
	
	else if (signal(SIGALRM, signal_handler) == SIG_ERR ||
	         signal(SIGHUP,  signal_handler) == SIG_ERR)
		perror("Failed to set signal handlers");
	
	else while(time(0) < end && getppid() == ppid) {
		int duration = end - time(0);
		
		alarm(MIN(10, MAX(duration, 1)));
		
		if (flock(lockfd, LOCK_EX) == -1) {
			if (errno == EINTR) continue;
			perror("Failed to lock lockfile");
			break;
		}
		
		signal(SIGALRM, SIG_IGN);
		
		/* write pid file */
		if (ftruncate(lockfd, 0) == -1) {
			perror("Failed to truncate lockfile");
			break;
		}
		
		char buf[7];
		_lucid_snprintf(buf, 7, "%d\n", getpid());
		
		if (write(lockfd, buf, 7) == -1) {
			perror("Failed to write pid to lockfile");
			break;
		}
		
		write(syncfd, "true", strlen("true"));
		close(syncfd);
		
		while(getppid() == ppid) sleep(10);
		
		exit(EXIT_SUCCESS);
	}
	
	if (syncfd != -1)
		write(syncfd, "false", strlen("false"));
	
	exit(EXIT_FAILURE);
}
