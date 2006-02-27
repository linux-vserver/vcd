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

#include <stdlib.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <errno.h>
#include <wait.h>
#include <termios.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <lucid/argv.h>
#include <lucid/mmap.h>
#include <lucid/open.h>
#include <lucid/sys.h>

#include "vc.h"

#include "commands.h"
#include "pathconfig.h"

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

static char *login_rcsid = "$Id: start.c 112 2006-02-21 16:27:58Z hollow $";
static xid_t login_xid   = 0;
static char *login_name  = NULL;

void login_usage(int rc)
{
	vc_printf("login: Start a virtual server.\n"
	          "usage: login <name>\n"
	          "\n"
	          "%s\n", login_rcsid);
	
	exit(rc);
}

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
	vc_printf("\n"); /* for cosmetic reasons */
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
		vc_abortp("Failed to read from terminal");
	
	/* get the current terminal settings */
	if (tcgetattr(t.fd, &t.term) == -1)
		vc_abortp("Failed to get terminal attributes");
	
	/* set the current terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSANOW, &t.term) == -1)
		vc_abortp("Failed to set terminal attributes");
	
	/* write activity to user */
	if (write(STDOUT_FILENO, buf, len) == -1)
		vc_abortp("Failed to write to stdout");
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
		vc_abortp("Failed to read from stdin");
	
	/* write activity to terminal */
	if (write(t.fd, buf, len) == -1)
		vc_abortp("Failed to write to terminal");
}

static
void login_sighandler(int sig)
{
	int status;
	
	signal(sig, SIG_DFL);
	
	switch(sig) {
		/* catch interrupt */
		case SIGINT:
			terminal_kill(sig);
			break;
		
		/* terminal died */
		case SIGCHLD:
			if (wait(&status) == -1)
				vc_abortp("waitpid");
			
			if (WIFEXITED(status))
				exit(WEXITSTATUS(status));
			
			if (WIFSIGNALED(status))
				kill(getpid(), WTERMSIG(status));
		
		/* window size has changed */
		case SIGWINCH:
			signal(SIGWINCH, login_sighandler);
			terminal_redraw();
			break;
		
		case SIGABRT:
			exit(EXIT_FAILURE);
		
		default:
			kill(getpid(), sig);
			break;
	}
}

void login_main(int argc, char *argv[])
{
	VC_INIT_ARGV0
	
	if (argc < 2)
		login_usage(EXIT_FAILURE);
	
	login_name = argv[1];
	
	/* 0) load configuration */
	if (vc_cfg_get_int(login_name, "vx.id", (int *) &login_xid) == -1)
		vc_abortp("vc_cfg_get_int(vx.id)");
	
	char *shell;
	
	if (vc_cfg_get_str(login_name, "vps.shell", &shell) == -1)
		shell = "/bin/bash";
	
	/* 1) migrate to context/namespace */
	if (nx_migrate(login_xid) == -1)
		vc_abortp("nx_migrate");
	
	if (vx_enter_namespace(login_xid) == -1)
		vc_abortp("vx_enter_namespace");
	
	if (vx_migrate(login_xid) == -1)
		vc_abortp("vx_migrate");
	
	/* 2) set terminal to raw mode */
	atexit(terminal_atexit);
	if (terminal_raw() == -1)
		vc_abortp("Failed to set terminal to raw mode");
	
	/* 3) fork new pseudo terminal */
	int slave;
	
	if (openpty(&t.fd, &slave, NULL, NULL, NULL) == -1)
		vc_abortp("Failed to open new pseudo terminal");
	
	/* 4) chroot to vdir */
	int vdirfd;
	char *buf;
	
	if (vc_cfg_get_str(login_name, "ns.root", &buf) == -1) {
		vc_asprintf(&buf, "%s/%s", __VDIRBASE, login_name);
		vdirfd = open(buf, O_RDONLY|O_DIRECTORY);
		free(buf);
	}
	
	else
		vdirfd = open(buf, O_RDONLY|O_DIRECTORY);
	
	if (vdirfd == -1)
		vc_abortp("open(vdirfd)");
	
	if (fchdir(vdirfd) == -1)
		vc_abortp("fchdir(vdirfd)");
	
	if (chroot(".") == -1)
		vc_abortp("chroot");
	
	/* 5) fork new terminal */
	pid_t pid;
	pid = fork();
	
	signal(SIGINT,   login_sighandler);
	signal(SIGCHLD,  login_sighandler);
	signal(SIGWINCH, login_sighandler);
	
	if (pid == -1)
		vc_abortp("Failed to fork new pseudo terminal");
	
	if (pid == 0) {
		/* we don't need the master side of the terminal */
		close(t.fd);
		
		/* login_tty() stupid dietlibc doesn't have it */
		setsid();
		
		if (ioctl(slave, TIOCSCTTY, NULL) == -1)
			vc_abortp("Failed to set controlling terminal");
		
		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);
		
		if (slave > 2)
			close(slave);
		
		int ac;
		char **av;
		
		if (argv_parse(shell, &ac, &av) == -1)
			vc_abortp("Invalid shell");
		
		if (execvp(av[0], av) == -1)
			vc_abortp("execvp");
	}
	
	/* save terminals pid */
	t.pid = pid;
	
	/* we want a redraw */
	terminal_redraw();
	
	/* 6) main loop */
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
			vc_abortp("Failed to select");
		}
		
		if (FD_ISSET(STDIN_FILENO, &rfds))
			user_activity();
		
		if (FD_ISSET(t.fd, &rfds))
			terminal_activity();
	}
}
