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

#ifndef _INTERNAL_PRINTF_H
#define _INTERNAL_PRINTF_H

#include <unistd.h>
#include <stdarg.h>

int vu_vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int vu_vasprintf(char **ptr, const char *fmt, va_list ap);
int vu_snprintf(char *str, size_t size, const char *fmt, /*args*/ ...);
int vu_asprintf(char **ptr, const char *fmt, /*args*/ ...);
int vu_vdprintf(int fd, const char *fmt, va_list ap);
int vu_dprintf(int fd, const char *fmt, /*args*/ ...);
int vu_vprintf(const char *fmt, va_list ap);
int vu_printf(const char *fmt, /*args*/ ...);

#endif
