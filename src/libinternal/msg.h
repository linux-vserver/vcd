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

#ifndef _INTERNAL_MSG_H
#define _INTERNAL_MSG_H

#include <unistd.h>
#include <string.h>

#include "printf.h"

/* save argv[0] for error functions below */
static const char *argv0;

/* warnings */
#define warn(fmt, ...) vu_dprintf(STDERR_FILENO, \
                                  "%s: " fmt "\n", \
                                  argv0, \
                                  ## __VA_ARGS__)

#define warnf(fmt, ...) warn("%s(): " fmt, \
                             __FUNCTION__, \
                             ## __VA_ARGS__)

#define warnp(fmt, ...)  warn(fmt ": %s", ## __VA_ARGS__ , strerror(errno))
#define warnfp(fmt, ...) warnf(fmt ": %s", ## __VA_ARGS__ , strerror(errno))

/* errors */
#define _err(wfunc, fmt, ...) do { \
	wfunc(fmt, ## __VA_ARGS__); \
	exit(EXIT_FAILURE); \
} while(0)

#define err(fmt, ...)  _err(warn, fmt, ## __VA_ARGS__)
#define errf(fmt, ...) _err(warnf, fmt, ## __VA_ARGS__)
#define errp(fmt, ...) _err(warnp, fmt, ## __VA_ARGS__)
#define errfp(fmt, ...) _err(warnfp, fmt, ## __VA_ARGS__)

#endif
