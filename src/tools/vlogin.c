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
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <errno.h>
#include <pty.h>
#include <vserver.h>

#include "printf.h"
#include "tools.h"

#define NAME  "vlogin"
#define DESCR "Context Login"

#define SHORT_OPTS "n:x:N"

struct options {
	GLOBAL_OPTS;
	nid_t nid;
	xid_t xid;
	bool namespace;
};

#define TS_RESET 0
#define TS_RAW   1

struct terminal {
	int fd;                /* terminal file descriptor */
	struct termios term;   /* terminal settings */
	struct winsize ws;     /* terminal size */
	pid_t pid;             /* terminal process id */
	struct termios termo;  /* original terminal settings */
	int state;             /* terminal state */
};

static struct terminal t;

static inline
void cmd_help()
{
	vu_printf("Usage: %s <opts>* [-- <shell> <args>*]\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -N            Also change namespace\n"
	       "    -n <xid>      Network Context ID\n"
	       "    -x <xid>      Context ID\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

/* set terminal to raw mode */
static
int terminal_raw(void)
{
	struct termios buf;
	
	/* save original terminal settings */
	if (tcgetattr(STDIN_FILENO, &t.termo) == -1)
		return -1;
	
	buf = t.termo;
	
	/* echo off
	** canonical mode off
	** extended input processing off
	** signal chars off */
	buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	
	/* no SIGINT on BREAK
	** CR-to-NL off
	** input parity check off
	** don't strip 8th bit on input
	** output flow control off */
	buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	
	/* clear size bits
	** parity checking off */
	buf.c_cflag &= ~(CSIZE | PARENB);
	
	/* set 8 bits/char */
	buf.c_cflag |= CS8;
	
	/* output processing off */
	buf.c_oflag &= ~(OPOST);
	
	/* 1 byte at a time
	** no timer */
	buf.c_cc[VMIN] = 1;
	buf.c_cc[VTIME] = 0;
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &buf) == -1)
		return -1;
	
	t.state = TS_RAW;
	
	return 0;
}

/* reset terminal to original state */
static
int terminal_reset(void)
{
	if (t.state != TS_RAW)
		return 0;
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t.termo) == -1)
		return -1;
	
	t.state = TS_RESET;
	
	return 0;
}

/* exit handler */
static
void terminal_atexit(void)
{
	terminal_reset();
	vu_printf("\n"); /* for cosmetic reasons */
}

/* send signal to terminal */
static
void terminal_kill(int sig)
{
	pid_t pgrp = -1;
	
	/* try to get process group leader */
	if (ioctl(t.fd, TIOCGPGRP, &pgrp) >= 0 &&
	    pgrp != -1 &&
	    kill(-pgrp, sig) != -1)
		return;
	
	/* fallback using terminal pid */
	kill(-t.pid, sig);
}

/* redraw the terminal screen */
static
void terminal_redraw(void)
{
	/* get winsize from stdin */
	ioctl(0, TIOCGWINSZ, &t.ws);
	
	/* set winsize in terminal */
	ioctl(t.fd, TIOCSWINSZ, &t.ws);
	
	/* set winsize change signal to terminal */
	terminal_kill(SIGWINCH);
}

/* copy terminal activity to user */
static
void terminal_activity(void)
{
	char buf[64];
	int len;
	
	/* read terminal activity */
	len = read(t.fd, buf, sizeof(buf));
	
	/* terminal died or strange things happened */
	if (len == -1)
		PEXIT("Failed to read from terminal", EXIT_COMMAND);
	
	/* get the current terminal settings */
	if (tcgetattr(t.fd, &t.term) == -1)
		PEXIT("Failed to get terminal attributes", EXIT_COMMAND);
	
	/* set the current terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSANOW, &t.term) == -1)
		PEXIT("Failed to set terminal attributes", EXIT_COMMAND);
	
	/* write activity to user */
	if (write(STDOUT_FILENO, buf, len) == -1)
		PEXIT("Failed to write to stdout", EXIT_COMMAND);
}

/* copy user activity to terminal */
static
void user_activity(void)
{
	char buf[64];
	int len;
	
	/* read user activity */
	len = read(STDIN_FILENO, buf, sizeof(buf));
	
	/* the user process died or strange thins happened */
	if (len == -1)
		PEXIT("Failed to read from stdin", EXIT_COMMAND);
	
	/* write activity to terminal */
	if (write(t.fd, buf, len) == -1)
		PEXIT("Failed to write to terminal", EXIT_COMMAND);
}

/* catch signals */
static
void signal_handler(int sig)
{
	int status;
	
	switch(sig) {
		/* catch interrupt */
		case SIGINT:
			terminal_kill(sig);
			break;
		
		/* terminal died */
		case SIGCHLD:
			wait(&status);
			exit(status);
			break;
		
		/* window size has changed */
		case SIGWINCH:
			signal(SIGWINCH, signal_handler);
			terminal_redraw();
			break;
		
		default:
			exit(0);
	}
	
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.nid = 0,
		.xid = 0,
	};
	
	int c;

	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'n':
				opts.nid = (nid_t) atoi(optarg);
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			case 'N':
				opts.namespace = 1;
				break;

			DEFAULT_GETOPT
		}
	}
	
	if (opts.xid == 0)
		EXIT("Invalid xid", EXIT_USAGE);
	
	/* change namespace */
	if (opts.namespace) {
		char cwd[PATH_MAX];
		
		if (getcwd(cwd, PATH_MAX) == NULL)
			PEXIT("Failed to get cwd", EXIT_COMMAND);
		
		if (vx_enter_namespace(opts.xid) == -1)
			PEXIT("Failed to enter namespace", EXIT_COMMAND);
		
		if (chdir(cwd) == -1)
			PEXIT("Failed to restore cwd", EXIT_COMMAND);
	}
	
	/* enter context */
	if (opts.nid > 1 && nx_migrate(opts.nid) == -1)
		PEXIT("Failed to migrate to network context", EXIT_COMMAND);
	
	if (vx_migrate(opts.xid, NULL) == -1)
		PEXIT("Failed to migrate to context", EXIT_COMMAND);
	
	/* set terminal to raw mode */
	atexit(terminal_atexit);
	if (terminal_raw() == -1)
		PEXIT("Failed to set terminal to raw mode", EXIT_COMMAND);
	
	/* fork new pseudo terminal */
	int slave;
	
	if (openpty(&t.fd, &slave, NULL, NULL, NULL) == -1)
		PEXIT("Failed to open new pseudo terminal", EXIT_COMMAND);
	
	/* chroot to cwd */
	if (chdir(".") == -1)
		PEXIT("Failed to chdir to cwd", EXIT_COMMAND);
	
	if (chroot(".") == -1)
		PEXIT("Failed to chroot to cwd", EXIT_COMMAND);
	
	pid_t pid = fork();

	if (pid == -1)
		PEXIT("Failed to fork new pseudo terminal", EXIT_COMMAND);
	
	if (pid == 0) {
		sleep(0);
		
		signal(SIGWINCH, signal_handler);

		/* we don't need the master side of the terminal */
		close(t.fd);
		
		/* login_tty() stupid dietlibc doesn't have it */
		setsid();
		
		if (ioctl(slave, TIOCSCTTY, NULL) == -1)
			PEXIT("Failed to set controlling terminal", EXIT_COMMAND);
		
		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);
		
		if (slave > 2)
			close(slave);
		
		/* check shell */
		if (argc > optind) {
			VPRINTF(&opts, "Executing shell '%s'\n", argv[optind]);
			if (execv(argv[optind], argv+optind) == -1) {
				PEXIT("Failed to execute shell", EXIT_COMMAND);
			}
		} else {
			VPRINTF(&opts, "Executing default shell '%s'\n", "/bin/sh");
			if (execl("/bin/sh", "/bin/sh", "-l", NULL) == -1) {
				PEXIT("Failed to execute default shell", EXIT_COMMAND);
			}
		}
	} else {
		/* setup some signal handlers */
		signal(SIGINT, signal_handler);
		signal(SIGCHLD, signal_handler);
		signal(SIGWINCH, signal_handler);
	}
	
	/* save terminals pid */
	t.pid = pid;
	
	/* set process title for ps */
	int i, len;
	
	for (i = 1; i < argc; i++) {
		memset(argv[i], '\0', strlen(argv[i]));
	}
	
	len = strlen(argv[0]);
	memset(argv[0], '\0', len);
	strncpy(argv[0], "VLOGIN", len);
	
	/* we want a redraw */
	terminal_redraw();
	
	/* main loop */
	fd_set rfds;
	int n = 0;
	
	for(;;) {
		/* init file descriptors for select */
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(t.fd, &rfds);
		n = t.fd;
		
		/* wait for something to happen */
		while (select(n + 1, &rfds, NULL, NULL, NULL) == -1) {
			if (errno == EINTR || errno == EAGAIN) continue;
			PEXIT("Failed to select", EXIT_COMMAND);
		}
		
		if (FD_ISSET(STDIN_FILENO, &rfds))
			user_activity();
		
		if (FD_ISSET(t.fd, &rfds))
			terminal_activity();
	}
	
	/* never get here */
	return -1;
}
