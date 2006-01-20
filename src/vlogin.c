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
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <ncurses.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pty.h>
#include <vserver.h>

#include "tools.h"

#define NAME  "vlogin"
#define DESCR "Context Login"

#define SHORT_OPTS "x:"

struct options {
	nid_t nid;
	xid_t xid;
};

#define TS_RESET 0
#define TS_RAW   1

struct terminal {
	int fd;                /* terminal file descriptor */
	struct termios term;   /* terminal settings */
	struct winsize ws;     /* terminal size */
	pid_t pid;             /* terminal process id */
	xid_t xid;             /* terminal context id */
	struct termios termo;  /* original terminal settings */
	int state;             /* terminal state */
};

static struct terminal *t;

static inline
void cmd_help()
{
	printf("Usage: %s <opts>*\n"
	       "\n"
	       "Available options:\n"
	       "    -x <xid>      Context ID\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

/* allocate a terminal struct */
static
void terminal_alloc(void)
{
	t = malloc(sizeof(struct terminal));
	
	if (!t)
		PEXIT("Failed to allocate memory for terminal struct", EXIT_COMMAND);
	
	memset(t, 0, sizeof(struct terminal));
	
	/* store initial entries */
	t->fd  = -1;
	t->xid = -1;
	t->pid = -1;
	t->state = TS_RESET;
}

/* dellocate a terminal struct */
static
void terminal_dealloc(void)
{
	free(t);
}

static
int terminal_raw(void)
{
	struct termios buf;
	
	if (tcgetattr(STDIN_FILENO, &t->termo) == -1)
		return -1;
	
	buf = t->termo;
	
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
	
	t->state = TS_RAW;
	
	return 0;
}

static
int terminal_clear(void)
{
	return write(STDOUT_FILENO, "\33[H\33[J", 6);
}

static
int terminal_reset(void)
{
	if (t->state != TS_RAW)
		return 0;
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t->termo) == -1)
		return -1;
	
	t->state = TS_RESET;
	
	return 0;
}

static
void terminal_atexit(void)
{
	terminal_reset();
	terminal_dealloc();
	printf("\n");
}

static
void terminal_kill(int sig)
{
	pid_t pgrp = -1;
	
	if (ioctl(t->fd, TIOCGPGRP, &pgrp) >= 0 &&
	    pgrp != -1 &&
	    kill(-pgrp, sig))
		return;
	
	/* fallback using terminal pid */
	kill(-t->pid, sig);
}

static
void terminal_redraw(void)
{
	ioctl(0, TIOCGWINSZ, &t->ws);
	ioctl(t->fd, TIOCSWINSZ, &t->ws);
	terminal_kill(SIGWINCH);
}

static
void terminal_activity(void)
{
	char buf[64];
	int len;
	
	/* read terminal activity */
	len = read(t->fd, buf, sizeof(buf));
	
	/* unlink terminal on error */
	if (len == -1)
		PEXIT("Failed to read from terminal", EXIT_COMMAND);
	
	/* Get the current terminal settings. */
	if (tcgetattr(t->fd, &t->term) == -1)
		PEXIT("Failed to get terminal attributes", EXIT_COMMAND);
	
	/* write activity to clients */
	if (write(STDOUT_FILENO, buf, len) == -1)
		PEXIT("Failed to write to stdout", EXIT_COMMAND);
}

static
void user_activity(void)
{
	char buf[64];
	int len;
	
	/* read client activity */
	len = read(STDIN_FILENO, buf, sizeof(buf));
	
	/* unlink terminal on error */
	if (len == -1)
		PEXIT("Failed to read from stdin", EXIT_COMMAND);
	
	/* write activity to terminal */
	if (write(t->fd, buf, len) == -1)
		PEXIT("Failed to write to terminal", EXIT_COMMAND);
}

static
void signal_handler(int sig)
{
	int status;
	
	switch(sig) {
		case SIGCHLD:
			wait(&status);
			exit(status);
			break;
		
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
		.nid = 0,
		.xid = 0,
	};
	
	int c;
	
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
			
			DEFAULT_GETOPT
		}
	}
	
	if (opts.xid == 0)
		EXIT("Invalid xid", EXIT_USAGE);
	
	/* allocate memory for terminal struct */
	terminal_alloc();
	atexit(terminal_atexit);
	
	if (vx_enter_namespace(opts.xid) == -1)
		PEXIT("Failed to enter namespace", EXIT_COMMAND);
	
	if (opts.nid != 0 && nx_migrate(opts.xid) == -1)
		PEXIT("Failed to migrate to network context", EXIT_COMMAND);
	
	if (vx_migrate(opts.xid) == -1)
		PEXIT("Failed to migrate to context", EXIT_COMMAND);
	
	/* set terminal to raw mode */
	if (terminal_raw() == -1)
		PEXIT("Failed to set terminal to raw mode", EXIT_COMMAND);
	
	/* setup some signal handler */
	signal(SIGCHLD, signal_handler);
	signal(SIGWINCH, signal_handler);
	
	/* fork new pseudo terminal */
	pid_t pid;
	pid = forkpty(&t->fd, NULL, NULL, NULL);
	
	if (pid == -1)
		PEXIT("Failed to fork new pseudo terminal", EXIT_COMMAND);
	
	if (pid == 0) {
		if (execl("/bin/sh", "/bin/sh", "-l", NULL) == -1)
			PEXIT("Failed to execute default shell", EXIT_COMMAND);
	}
	
	t->pid = pid;
	
	/* we want a redraw */
	terminal_redraw();
	
	/* main loop */
	fd_set rfds;
	int n = 0;
	
	for(;;) {
		/* init file descriptors for select */
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(t->fd, &rfds);
		n = t->fd;
		
		/* wait for something to happen */
		while (select(n + 1, &rfds, NULL, NULL, NULL) == -1) {
			if (errno == EINTR || errno == EAGAIN) continue;
			PEXIT("Failed to select", EXIT_COMMAND);
		}
		
		if (FD_ISSET(STDIN_FILENO, &rfds))
			user_activity();
		
		if (FD_ISSET(t->fd, &rfds))
			terminal_activity();
	}
	
	/* never get here */
	return -1;
}
