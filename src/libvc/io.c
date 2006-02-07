/***************************************************************************
 *   Copyright 2005-2006 by the vserver-utils team                         *
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
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#include <libowfat/fmt.h>
#include <libowfat/str.h>

#include "vc.h"

/* libowfat misses prototype for fmt_8longlong */
unsigned int fmt_8longlong(char *dest, unsigned long long i);

/* format structs */
typedef struct {
	bool alt;
	bool zero;
	bool left;
	bool blank;
	bool sign;
} __flags_t;

typedef struct {
	bool isset;
	unsigned int width;
} __prec_t;

typedef struct {
	__flags_t f;
	unsigned int w;
	__prec_t  p;
	unsigned char l;
	unsigned char c;
} __printf_t;

/* supported formats:
** - format flags: #, 0, -, ' ', +
** - field width
** - argument precision
** - length mods: hh, h, l, z (handled as long); ll (handled as long long)
** - conversion spec: d, i, u, o, x, f, c, s, p */
int vc_vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	/* keep track of string length */
	size_t idx = 0;
	
	while (*fmt) {
		if (*fmt != '%') {
			if (idx < size)
				str[idx++] = *fmt++;
			else {
				idx++;
				*fmt++;
			}
		}
		
		else {
			/* skip % */
			*fmt++;
			
			/* format struct */
			__printf_t f;
			
			/* sanitize struct with default values */
			f.f.alt   = false;
			f.f.zero  = false;
			f.f.left  = false;
			f.f.blank = false;
			f.f.sign  = false;
			
			f.w = 0;
			
			f.p.isset = false;
			f.p.width = 1;
			
			f.l = '\0';
			f.c = '\0';
			
			/* parse flags */
			while (*fmt == '0' || *fmt == '-' || *fmt == '+' || *fmt == ' ' || *fmt == '#') {
				switch (*fmt) {
					case '#': f.f.alt   = true; break;
					case '0': f.f.zero  = true; break;
					case '-': f.f.left  = true; break;
					case ' ': f.f.blank = true; break;
					case '+': f.f.sign  = true; break;
				}
				
				*fmt++;
			}
			
			/* parse field width */
			if (*fmt == '*') {
				*fmt++;
				
				int arg = va_arg(ap, int);
				
				if (arg >= 0)
					f.w = arg;
				else {
					f.w = -arg;
					f.f.left = true;
				}
			} else if (isdigit(*fmt)) {
				f.w = *fmt++ - '0';
				
				while (isdigit(*fmt))
					f.w = 10 * f.w + (unsigned int)(*fmt++ - '0');
			}
			
			/* parse precision */
			if (*fmt == '.') {
				*fmt++;
				
				f.p.isset = true;
				f.p.width = 0;
				
				if (*fmt == '*') {
					*fmt++;
					
					int arg = va_arg(ap, int);
					
					if (arg >= 0)
						f.p.width = arg;
				} else if (isdigit(*fmt)) {
					f.p.width = *fmt++ - '0';
					
					while (isdigit(*fmt))
						f.p.width = 10 * f.p.width + (unsigned int)(*fmt++ - '0');
				}
			}
			
			/* parse length modifier
			** hh, h and l, z are all handled as long int
			** ll, L is handled as long long int */
			switch (*fmt) {
				case 'h':
				case 'z':
					f.l = 'l';
					
					if (*(fmt+1) == 'h')
						*fmt++;
					
					break;
				
				case 'l':
					f.l = 'l';
					
					if (*(fmt+1) == 'l') {
						f.l = '2';
						*fmt++;
					}
					
					break;
				
				case 'L':
					f.l = '2';
					break;
				
				default: break;
			}
			
			if (f.l != '\0')
				*fmt++;
			
			/* parse conversion specifier */
			switch (*fmt) {
				case 'd':
				case 'i':
					f.c = 'd';
					break;
				
				case 'f':
				
				case 'o':
				case 'u':
				case 'x':
				
				case 'c':
				case 's':
				
				case 'p':
				
				case '%':
					f.c = *fmt;
					break;
				
				default: break;
			}
			
			*fmt++;
			
			/* sign overrides blank */
			if (f.f.sign)
				f.f.blank = false;
			
			/* left overrides zero */
			if (f.f.left)
				f.f.zero = false;
			
			/* no zero padding if precision is specified */
			if (f.p.isset && f.w > 0)
				f.f.zero = false;
			
			/* no zero padding for string conversions */
			if (f.c == 'c' || f.c == 's')
				f.f.zero = false;
			
			/* signed long argument */
			signed long darg; signed long long ldarg;
			
			/* unsigned long argument */
			unsigned long uarg; unsigned long long luarg;
			
			/* float argument */
			double farg;
			
			/* character argument */
			int carg;
			
			/* string argument */
			char *sarg;
			
			/* pointer argument */
			void *parg;
			
			/* used for precision counting */
			int len;
			
			/* temporary buffer for integer conversions */
			char ibuf[64];
			
			/* temporary buffer for string conversion */
			char *buf = ibuf;
			
			/* buffer length */
			size_t buflen = 0;
			
			/* do conversions */
			switch (f.c) {
				case 'd': /* signed conversion */
					if (f.l == '2') { /* long long */
						ldarg = va_arg(ap, signed long long);
						
						if (ldarg == 0 && f.p.isset && f.p.width == 0)
							break;
						
						/* forced sign */
						if (f.f.sign) {
							if (f.f.blank || ldarg == 0)
								buflen += fmt_str(&ibuf[buflen], " ");
							else
								buflen += fmt_plusminus(&ibuf[buflen], ldarg);
						} else {
							buflen += fmt_minus(&ibuf[buflen], ldarg);
						}
						
						len = fmt_ulonglong(&ibuf[buflen], (unsigned long long) ldarg);
						
						while ((unsigned long) len < f.p.width) {
							buflen += fmt_str(&ibuf[buflen], "0");
							f.p.width--;
						}
					
						buflen += fmt_ulonglong(&ibuf[buflen], (unsigned long long) ldarg);
					} else { /* !long long */
						darg = va_arg(ap, signed long);
						
						if (darg == 0 && f.p.isset && f.p.width == 0)
							break;
						
						/* forced sign */
						if (f.f.sign) {
							if (f.f.blank || darg == 0)
								buflen += fmt_str(&ibuf[buflen], " ");
							else
								buflen += fmt_plusminus(&ibuf[buflen], darg);
						} else {
							buflen += fmt_minus(&ibuf[buflen], darg);
						}
						
						buflen += fmt_ulong0(&ibuf[buflen], (unsigned long) darg, f.p.width);
					}
					break;
				
				case 'f': /* float conversion */
					farg = va_arg(ap, double);
					
					if (!f.p.isset)
						f.p.width = 6;
					
					/* forced sign */
					if (f.f.sign) {
						if (f.f.blank || farg == 0)
							buflen += fmt_str(&ibuf[buflen], " ");
						else
							buflen += fmt_plusminus(&ibuf[buflen], farg);
					} else {
						buflen += fmt_minus(&ibuf[buflen], farg);
					}
					
					len = fmt_double(FMT_LEN, farg, 64 - buflen, f.p.width);
					
					while ((unsigned long) len < f.p.width) {
						buflen += fmt_str(&ibuf[buflen], "0");
						f.p.width--;
					}
					
					buflen += fmt_double(&ibuf[buflen], farg, 64 - buflen, f.p.width);
					break;
				
				/* unsigned conversions */
				case 'o': /* octal representation */
					if (f.l == '2') { /* long long */
						uarg = va_arg(ap, unsigned long);
						
						if (uarg == 0 && f.p.isset && f.p.width == 0)
							break;
						
						if (f.f.alt)
							buflen += fmt_str(&ibuf[buflen], "0");
						
						len = fmt_8long(FMT_LEN, uarg);
						
						while ((unsigned long) len < f.p.width) {
							buflen += fmt_str(&ibuf[buflen], "0");
							f.p.width--;
						}
						
						buflen += fmt_8long(&ibuf[buflen], uarg);
					} else { /* !long long */
						luarg = va_arg(ap, unsigned long long);
						
						if (luarg == 0 && f.p.isset && f.p.width == 0)
							break;
						
						if (f.f.alt)
							buflen += fmt_str(&ibuf[buflen], "0");
						
						len = fmt_8longlong(FMT_LEN, luarg);
						
						while ((unsigned long) len < f.p.width) {
							buflen += fmt_str(&ibuf[buflen], "0");
							f.p.width--;
						}
						
						buflen += fmt_8longlong(&ibuf[buflen], luarg);
					}
					
					break;
				
				case 'u': /* decimal representation */
					if (f.l == '2') {
						luarg = va_arg(ap, unsigned long long);
						
						if (luarg == 0 && f.p.isset && f.p.width == 0)
							break;
						
						len = fmt_ulonglong(&ibuf[buflen], luarg);
						
						while ((unsigned long) len < f.p.width) {
							buflen += fmt_str(&ibuf[buflen], "0");
							f.p.width--;
						}
					
						buflen += fmt_ulonglong(&ibuf[buflen], luarg);
					} else {
						uarg = va_arg(ap, unsigned long);
						
						if (uarg == 0 && f.p.isset && f.p.width == 0)
							break;
						
						buflen += fmt_ulong0(&ibuf[buflen], uarg, f.p.width);
					}
					break;
				
				case 'x': /* hex conversion */
					if (f.l == '2') {
						luarg = va_arg(ap, unsigned long long);
						
						if (luarg == 0 && f.p.isset && f.p.width == 0)
							break;
						
						if (f.f.alt)
							buflen += fmt_str(&ibuf[buflen], "0x");
						
						len = fmt_xlonglong(FMT_LEN, luarg);
						
						while ((unsigned long) len < f.p.width) {
							buflen += fmt_str(&ibuf[buflen], "0");
							f.p.width--;
						}
						
						buflen += fmt_xlonglong(&ibuf[buflen], luarg);
					} else {
						uarg = va_arg(ap, unsigned long);
						
						if (uarg == 0 && f.p.isset && f.p.width == 0)
							break;
						
						if (f.f.alt)
							buflen += fmt_str(&ibuf[buflen], "0x");
						
						len = fmt_xlong(FMT_LEN, uarg);
						
						while ((unsigned long) len < f.p.width) {
							buflen += fmt_str(&ibuf[buflen], "0");
							f.p.width--;
						}
						
						buflen += fmt_xlong(&ibuf[buflen], uarg);
					}
					break;
				
				case 'c': /* character conversion */
					carg = va_arg(ap, int);
					ibuf[buflen++] = (char) carg;
					break;
				
				case 's': /* string conversion */
					sarg = va_arg(ap, char *);
					buf = sarg;
					
					if (!buf) buflen = 0;
					else if (!f.p.isset) buflen = str_len(buf);
					else if (f.p.width == 0) buflen = 0;
					else {
						const char *q = memchr(buf, '\0', f.p.width);
						
						if (q == 0) /* string will be truncated */
							buflen = f.p.width;
						else
							buflen = q - buf;
					}
					
					break;
					
				case 'p': /* pointer conversion */
					parg = va_arg(ap, void *);
					
					if (f.f.alt)
						buflen += fmt_str(&ibuf[buflen], "0x");
					
					buflen += fmt_xlong(&ibuf[buflen], (unsigned long) parg);
					break;
				
				default: break;
			}
			
			/* blank/zero padding using right alignment */
			if (!f.f.left) {
				while (f.w > buflen) {
					if (idx < size) {
						if (f.f.zero)
							fmt_strn(&str[idx++], "0", 1);
						else
							fmt_strn(&str[idx++], " ", 1);
					} else
						idx++;
					
					f.w--;
				}
			}
			
			size_t i;
			
			/* write the converted argument */
			for (i = 0; i < buflen; i++) {
				if (idx < size)
					fmt_strn(&str[idx++], &buf[i], 1);
				else
					idx++;
			}
			
			/* blank/zero padding using left alignment */
			if (f.f.left && f.w > i) {
				while (f.w > buflen) {
					if (idx < size) {
						if (f.f.zero)
							fmt_strn(&str[idx++], "0", 1);
						else
							fmt_strn(&str[idx++], " ", 1);
					} else
						idx++;
					
					f.w--;
				}
			}
		}
	}
	
	if (idx < size)
		str[idx] = '\0';
	
	return idx;
}

int vc_vasprintf(char **ptr, const char *fmt, va_list ap)
{
	va_list ap2;
	int len;
	
	*ptr = NULL;
	
	/* don't consume the original ap, we'll need it again */
	va_copy(ap2, ap);
	
	/* get required size */
	len = vc_vsnprintf(FMT_LEN, 0, fmt, ap2);
	
	va_end(ap2);
	
	/* if size is 0, no buffer is allocated
	** just set *ptr to NULL and return size */
	if (len > 0) {
		*ptr = (char *) malloc(len + 1);
		
		if (*ptr == NULL) {
			errno = ENOMEM;
			return -1;
		}
		
		vc_vsnprintf(*ptr, len + 1, fmt, ap);
		return len;
	}
	
	return len;
}

int vc_snprintf(char *str, size_t size, const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	return vc_vsnprintf(str, size, fmt, ap);
}

int vc_asprintf(char **ptr, const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	return vc_vasprintf(ptr, fmt, ap);
}

int vc_vdprintf(int fd, const char *fmt, va_list ap)
{
	char *buf;
	int buflen, len;
	
	buflen = vc_vasprintf(&buf, fmt, ap);
	len = write(fd, buf, buflen + 1);
	free(buf);
	
	return len;
}

int vc_dprintf(int fd, const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	return vc_vdprintf(fd, fmt, ap);
}

int vc_vprintf(const char *fmt, va_list ap)
{
	return vc_vdprintf(STDOUT_FILENO, fmt, ap);
}

int vc_printf(const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	return vc_vprintf(fmt, ap);
}

/* save argv[0] for error functions below */
const char *vc_argv0;

/* warnings */
void vc_warn(const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	vc_dprintf(STDERR_FILENO, "%s: ", vc_argv0);
	vc_vdprintf(STDERR_FILENO, fmt, ap);
	vc_dprintf(STDERR_FILENO, "\n");
}

void vc_warnp(const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	vc_dprintf(STDERR_FILENO, "%s: ", vc_argv0);
	vc_vdprintf(STDERR_FILENO, fmt, ap);
	vc_dprintf(STDERR_FILENO, ": %s\n", strerror(errno));
}

void vc_err(const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	vc_dprintf(STDERR_FILENO, "%s: ", vc_argv0);
	vc_vdprintf(STDERR_FILENO, fmt, ap);
	vc_dprintf(STDERR_FILENO, "\n");
	
	exit(EXIT_FAILURE);
}

void vc_errp(const char *fmt, /*args*/ ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	vc_dprintf(STDERR_FILENO, "%s: ", vc_argv0);
	vc_vdprintf(STDERR_FILENO, fmt, ap);
	vc_dprintf(STDERR_FILENO, ": %s\n", strerror(errno));
	
	exit(EXIT_FAILURE);
}
