// Copyright 2007 Benedikt Böhm <hollow@gentoo.org>
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

#if   defined(__alpha__)
#include "asm-alpha/unistd.h"
#elif defined(__arm__)
#include "asm-arm/unistd.h"
#elif defined(__cris__)
#include "asm-cris/unistd.h"
#elif defined(__frv__)
#include "asm-frv/unistd.h"
#elif defined(__H8300__)
#include "asm-h8300/unistd.h"
#elif defined(__i386__)
#include "asm-i386/unistd.h"
#elif defined(__ia64__)
#include "asm-ia64/unistd.h"
#elif defined(__M32R__)
#include "asm-m32r/unistd.h"
#elif defined(__m68000__)
#include "asm-m68k/unistd.h"
#elif defined(__mips__)
#include "asm-mips/unistd.h"
#elif defined(__hppa__)
#include "asm-parisc/unistd.h"
#elif defined(__powerpc__)
#include "asm-powerpc/unistd.h"
#elif defined(__s390__)
#include "asm-s390/unistd.h"
#elif defined(__sh__) && defined(__SH5__)
#include "asm-sh64/unistd.h"
#elif defined(__sh__) && !defined(__SH5__)
#include "asm-sh/unistd.h"
#elif defined(__sparc__) && defined(__arch64__)
#include "asm-sparc64/unistd.h"
#elif defined(__sparc__) && !defined(__arch64__)
#include "asm-sparc/unistd.h"
#elif defined(__v850__)
#include "asm-v850/unistd.h"
#elif defined(__x86_64__)
#include "asm-x86_64/unistd.h"
#else
#error unsupported cpu architecture
#endif

#include "syscall.h"
#include "syscall-compat.h"

#ifndef HAVE_FCHMODAT
_syscall4(int, fchmodat,
		int, dirfd,
		const char *, pathname,
		mode_t, mode,
		int, flags)
#endif

#ifndef HAVE_FCHOWNAT
_syscall5(int, fchownat,
		int, dirfd,
		const char *, pathname,
		uid_t, owner,
		gid_t, group,
		int, flags)
#endif

#ifndef HAVE_LINKAT
_syscall5(int, linkat,
		int, olddirfd,
		const char *, oldpath,
		int, newdirfd,
		const char *, newpath,
		int, flags)
#endif

#ifndef HAVE_MKDIRAT
_syscall3(int, mkdirat,
		int, dirfd,
		const char *, pathname,
		mode_t, mode)
#endif

#ifndef HAVE_OPENAT
_syscall4(int, openat,
		int, dirfd,
		const char *, pathname,
		int, flags,
		mode_t, mode)
#endif

#ifndef HAVE_SYMLINKAT
_syscall3(int, symlinkat,
		const char *, oldpath,
		int, newdirfd,
		const char *, newpath)
#endif
