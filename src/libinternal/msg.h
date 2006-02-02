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

extern const char *argv0;

void warn(const char *fmt, /*args*/ ...);
void warnp(const char *fmt, /*args*/ ...);
void err(const char *fmt, /*args*/ ...);
void errp(const char *fmt, /*args*/ ...);

#define warnf(fmt, ...)  warn("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define warnfp(fmt, ...) warnp("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define errf(fmt, ...)   err("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)
#define errfp(fmt, ...)  errp("%s(): " fmt, __FUNCTION__, ## __VA_ARGS__)

#endif
