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

#include "lucid.h"

#include "cfg.h"
#include "log.h"

static int log_fd     = -1;
static int log_level  = 3;
static int log_stderr = 0;
static const char *log_ident = NULL;

int log_init(const char *ident, int debug)
{
	int rc = 0;
	char *logfile, *logdir = cfg_getstr(cfg, "log-dir");
	
	if (!ident || !*ident)
		return errno = EINVAL, -1;
	
	log_ident = ident;
	
	if (!debug && str_isempty(logdir))
		return errno = ENOENT, -1;
	
	if (debug)
		log_stderr = 1;
	
	logdir = realpath(logdir, NULL);
	
	if (!logdir)
		return -1;
	
	if (log_fd >= 0)
		close(log_fd);
	
	asprintf(&logfile, "%s/%s.log", logdir, ident);
	log_fd = open_append(logfile);
	
	if (log_fd == -1)
		rc = -1;
	else if (!log_stderr)
		log_level = cfg_getint(cfg, "log-level");
	
	free(logfile);
	free(logdir);
	
	return rc;
}

static
void _log_internal(int level, const char *fmt, va_list ap)
{
	time_t curtime = time(0);
	char *levelstr = NULL, timestr[64];
	
	if (level > log_level)
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
	
	if (log_fd > -1) {
		dprintf(log_fd, "[%s] [%5d] [%s] ", timestr, getpid(), levelstr);
		vdprintf(log_fd, fmt, ap);
		dprintf(log_fd, "\n");
	}
	
	if (log_stderr) {
		dprintf(STDERR_FILENO, "[%s] [%5d] [%s] [%s] ",
		        timestr, getpid(), levelstr, log_ident);
		vdprintf(STDERR_FILENO, fmt, ap);
		dprintf(STDERR_FILENO, "\n");
	}
}

void log_debug(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_DEBG, fmt, ap);
}

void log_info(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_INFO, fmt, ap);
}

void log_warn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_WARN, fmt, ap);
}

void log_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_ERR, fmt, ap);
}


void log_debug_and_die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_DEBG, fmt, ap);
	exit(EXIT_FAILURE);
}

void log_info_and_die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_INFO, fmt, ap);
	exit(EXIT_FAILURE);
}

void log_warn_and_die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_WARN, fmt, ap);
	exit(EXIT_FAILURE);
}

void log_error_and_die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_ERR, fmt, ap);
	exit(EXIT_FAILURE);
}
