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

#ifndef _VSHELPER_LOG_H
#define _VSHELPER_LOG_H

int  log_init (int debug);

void log_debug(const char *fmt, ...);
void log_info (const char *fmt, ...);
void log_warn (const char *fmt, ...);
void log_error(const char *fmt, ...);

void log_debug_and_die(const char *fmt, ...);
void log_info_and_die (const char *fmt, ...);
void log_warn_and_die (const char *fmt, ...);
void log_error_and_die(const char *fmt, ...);

#define TRACEIT log_debug("[trace] %s (%s:%d)", \
                          __FUNCTION__, __FILE__, __LINE__);

#endif