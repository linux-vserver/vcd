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
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "confuse.h"
#include "lucid.h"

#include "log.h"

extern cfg_t *cfg;

static int log_fd     = -1;
static int log_level  = 3;
static int log_stderr = 0;
static char *log_ident = NULL;

int log_init(char *ident, int debug)
{
	int level;
	char *logfile, *logdir = cfg_getstr(cfg, "log-dir");
	
	if (!ident || !*ident)
		return errno = EINVAL, -1;
	
	log_ident = ident;
	
	if (!logdir || !*logdir)
		return errno = ENOENT, -1;
	
	logdir = realpath(logdir, NULL);
	
	if (log_fd >= 0)
		close(log_fd);
	
	asprintf(&logfile, "%s/%s.log", logdir, ident);
	log_fd = open_append(logfile);
	free(logfile);
	
	if (log_fd == -1)
		return -1;
	
	level = cfg_getint(cfg, "log-level");
	
	if (!(level < 0 || level > 4))
		log_level = level;
	
	if (debug)
		log_stderr = 1;
	
	return 0;
}

static
void _log_internal(int level, char *fmt, va_list ap)
{
	time_t curtime = time(0);
	char *levelstr = NULL, timestr[64];
	
	if (log_fd == -1 || level > log_level)
		return;
	
	switch (level) {
	case LOG_DEBG:
		levelstr = "debug";
		break;
	
	case LOG_INFO:
		levelstr = "info";
		break;
	
	case LOG_WARN:
		levelstr = "warn";
		break;
	
	case LOG_ERR:
		levelstr = "error";
		break;
	}
	
	bzero(timestr, 64);
	strftime(timestr, 63, "%a %b %d %H:%M:%S %Y", localtime(&curtime));
	
	dprintf(log_fd, "[%s] [%5d] [%s] ", timestr, getpid(), levelstr);
	vdprintf(log_fd, fmt, ap);
	dprintf(log_fd, "\n");
	
	if (log_stderr) {
		dprintf(STDERR_FILENO, "[%s] [%5d] [%s] [%s] ",
		        timestr, getpid(), levelstr, log_ident);
		vdprintf(STDERR_FILENO, fmt, ap);
		dprintf(STDERR_FILENO, "\n");
	}
}

void log_debug(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_DEBG, fmt, ap);
}

void log_info(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_INFO, fmt, ap);
}

void log_warn(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_WARN, fmt, ap);
}

void log_error(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_ERR, fmt, ap);
}


void log_debug_and_die(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_DEBG, fmt, ap);
	exit(EXIT_FAILURE);
}

void log_info_and_die(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_INFO, fmt, ap);
	exit(EXIT_FAILURE);
}

void log_warn_and_die(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_WARN, fmt, ap);
	exit(EXIT_FAILURE);
}

void log_error_and_die(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_ERR, fmt, ap);
	exit(EXIT_FAILURE);
}
