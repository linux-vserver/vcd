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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <errno.h>
#include <pty.h>
#include <vserver.h>

#include "tools.h"

struct terminal {
	int fd;                          /* terminal file descriptor */
	struct termios term;             /* terminal settings */
	struct winsize ws;               /* terminal size */
	pid_t pid;                       /* terminal process id */
	struct termios termo;            /* original terminal settings */
	enum { TS_RESET, TS_RAW } state; /* terminal state */
};

static struct terminal t;

/* set terminal to raw mode */
static
void terminal_raw(void)
{
	struct termios buf;
	
	/* save original terminal settings */
	if (tcgetattr(STDIN_FILENO, &t.termo) == -1)
		perr("tcgetattr");
	
	buf = t.termo;
	
	/* convert terminal settings to raw mode */
	cfmakeraw(&buf);
	
	/* apply raw terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &buf) == -1)
		perr("tcsetattr");
	
	t.state = TS_RAW;
}

/* reset terminal to original state */
static
void terminal_reset(void)
{
	if (t.state != TS_RAW)
		return;
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t.termo) == -1)
		perr("tcsetattr");
	
	t.state = TS_RESET;
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
	if (ioctl(0, TIOCGWINSZ, &t.ws) == -1)
		return;
	
	/* set winsize in terminal */
	ioctl(t.fd, TIOCSWINSZ, &t.ws);
	
	/* set winsize change signal to terminal */
	terminal_kill(SIGWINCH);
}

/* copy terminal activities */
static
void terminal_copy(int src, int dst)
{
	char buf[64];
	int len;
	
	/* read terminal activity */
	if ((len = read(src, buf, sizeof(buf))) == -1)
		perr("read");
	
	/* write terminal activity */
	if (write(dst, buf, len) == -1)
		perr("write");
}

/* shuffle all output, and reset the terminal */
static
void terminal_end(void)
{
	char buf[64];
	ssize_t len;
	long options;
	
	if ((options = fcntl(t.fd, F_GETFL, 0)) == -1)
		perr("fcntl");
	
	if (fcntl(t.fd, F_SETFL, options | O_NONBLOCK) == -1)
		perr("fcntl");
	
	while (1) {
		len = read(t.fd, buf, sizeof(buf));
		
		if (len == 0 || len == -1)
			break;
		
		if (write(STDOUT_FILENO, buf, len) == -1)
			perr("write");
	}
	
	/* in case atexit hasn't been setup yet */
	terminal_reset();
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
			terminal_end();
			wait(&status);
			exit(WEXITSTATUS(sig));
			break;
		
		/* window size has changed */
		case SIGWINCH:
			terminal_redraw();
			break;
		
		default:
			exit(0);
	}
}

void do_vlogin(int argc, char *argv[], int ind)
{
	int i, n, slave;
	pid_t pid;
	fd_set rfds;
	
	/* set terminal to raw mode */
	terminal_raw();
	
	/* fork new pseudo terminal */
	if (openpty(&t.fd, &slave, NULL, NULL, NULL) == -1)
		perr("openpty");
	
	/* setup SIGCHLD here, so we're sure to get the signal */
	signal(SIGCHLD, signal_handler);
	
	if ((pid = fork()) == -1)
		perr("fork");
	
	if (pid == 0) {
		/* we don't need the master side of the terminal */
		close(t.fd);
		
		/* login_tty() stupid dietlibc doesn't have it */
		if (setsid() == -1)
			perr("setsid");
		
		if (ioctl(slave, TIOCSCTTY, NULL) == -1)
			perr("ioctl");
		
		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);
		
		if (slave > 2)
			close(slave);
		
		/* check shell */
		if (argc > optind) {
			if (execv(argv[ind], argv+ind) == -1)
				perr("execv");
		} else {
			if (execl("/bin/sh", "/bin/sh", "-l", NULL) == -1)
				perr("execl");
		}
	}
	
	/* setup SIGINT and SIGWINCH here, as they can cause loops in the child */
	signal(SIGWINCH, signal_handler);
	signal(SIGINT, signal_handler);
	
	/* save terminals pid */
	t.pid = pid;
	
	/* set process title for ps */
	n = strlen(argv[0]);
	
	for (i = 0; i < argc; i++)
		bzero(argv[i], strlen(argv[i]));
	
	strcpy(argv[0], "login");
	
	/* reset terminal to its original mode */
	atexit(terminal_reset);
	
	/* we want a redraw */
	terminal_redraw();
	
	/* main loop */
	
	while (1) {
		/* init file descriptors for select */
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(t.fd, &rfds);
		n = t.fd;
		
		/* wait for something to happen */
		while (select(n + 1, &rfds, NULL, NULL, NULL) == -1) {
			if (errno == EINTR || errno == EAGAIN) continue;
			perr("select");
		}
		
		if (FD_ISSET(STDIN_FILENO, &rfds))
			terminal_copy(STDIN_FILENO, t.fd);
		
		if (FD_ISSET(t.fd, &rfds))
			terminal_copy(t.fd, STDOUT_FILENO);
	}
	
	/* never get here, signal handler exits */
}
