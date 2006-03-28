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

/*
 * Some code taken from Herbet Poetzl's BME test app
 */
static const char *_err_name[] = {
	"SUCCESS",      "EPERM",        "ENOENT",       "ESRCH",
	"EINTR",	"EIO",  	"ENXIO",	"E2BIG",
	"ENOEXEC",	"EBADF",	"ECHILD",	"EAGAIN",
	"ENOMEM",	"EACCES",	"EFAULT",	"ENOTBLK",
	"EBUSY",	"EEXIST",	"EXDEV",	"ENODEV",
	"ENOTDIR",	"EISDIR",	"EINVAL",	"ENFILE",
	"EMFILE",	"ENOTTY",	"ETXTBSY",	"EFBIG",
	"ENOSPC",	"ESPIPE",	"EROFS",	"EMLINK",
	"EPIPE",	"EDOM", 	"ERANGE",	"EDEADLK",
	"ENAMETOOLONG", "ENOLCK",	"ENOSYS",	"ENOTEMPTY",
	"ELOOP",	"[41]", 	"ENOMSG",	"EIDRM",
	"ECHRNG",	"EL2NSYNC",	"EL3HLT",	"EL3RST",
	"ELNRNG",	"EUNATCH",	"ENOCSI",	"EL2HLT",
	"EBADE",	"EBADR",	"EXFULL",	"ENOANO",
	"EBADRQC",	"EBADSLT",	"[58]", 	"EBFONT",
	"ENOSTR",	"ENODATA",	"ETIME",	"ENOSR",
	"ENONET",	"ENOPKG",	"EREMOTE",	"ENOLINK",
	"EADV", 	"ESRMNT",	"ECOMM",	"EPROTO",
	"EMULTIHOP",	"EDOTDOT",	"EBADMSG",	"EOVERFLOW",
	"ENOTUNIQ",	"EBADFD",	"EREMCHG",	"ELIBACC",
	"ELIBBAD",	"ELIBSCN",	"ELIBMAX",	"ELIBEXEC",
	"EILSEQ",	"ERESTART",	"ESTRPIPE",	"EUSERS",
	"ENOTSOCK",	"EDESTADDRREQ", "EMSGSIZE",	"EPROTOTYPE",
	"ENOPROTOOPT",  "EPROTONOSUPPORT",	"ESOCKTNOSUPPORT",	"EOPNOTSUPP",
	"EPFNOSUPPORT", "EAFNOSUPPORT", "EADDRINUSE",	"EADDRNOTAVAIL",
	"ENETDOWN",	"ENETUNREACH",  "ENETRESET",	"ECONNABORTED",
	"ECONNRESET",	"ENOBUFS",	"EISCONN",	"ENOTCONN",
	"ESHUTDOWN",	"ETOOMANYREFS", "ETIMEDOUT",	"ECONNREFUSED",
	"EHOSTDOWN",	"EHOSTUNREACH", "EALREADY",	"EINPROGRESS",
	"ESTALE",	"EUCLEAN",	"ENOTNAM",	"ENAVAIL",
	"EISNAM",	"EREMOTEIO",	"EDQUOT",	"ENOMEDIUM",
	"EMEDIUMTYPE",  "[?]"
};

static inline const char *err_name(int errno)
{
	int index = ((errno <= 0) && (errno > -125)) ? -errno : 125;
	return _err_name[index];
}

#define report(a, b, c) { errno = 0; _report(a, b, c); }
void _report(const char *title, const char *file, int retval)
{
	fprintf(stdout, "[I]  %-14.14s\t%-32.32s %-20.20s ",
			err_name(-errno), title, file /* + bdir_len + 1 */);
//	if (show_time) {
//		static struct stat sbuf;
//
//		retval = lstat(file, &sbuf);
//		if (retval < 0)
//			fprintf(stdout, "(%s) ", err_name(retval));
//		else
//			fprintf(stdout, "(%d,%c,%c) ",
//					(int)sbuf.st_ctime,
//					delta_dir(sbuf.st_ctime, sbuf.st_mtime),
//					delta_dir(sbuf.st_mtime, sbuf.st_atime));
//	}
	fprintf(stdout, "[%s]\n", strerror(errno));
}

#define vreport(opts, ttl, f, retv) { errno = 0; _vreport((opts)->verbose, ttl, f, retv); }
void _vreport(bool verbose, const char *title, const char *file, int retval)
{
	if (verbose) {
		if (retval > 0) retval=0;
		fprintf(stdout, "[V]  %-14.14s\t%-32.32s %-20.20s ",
				err_name(-errno), title, file /* + bdir_len + 1 */);
		fprintf(stdout, "[%s]\n", strerror(errno));
	}
}

/*
 * Some defines and code imported from tools.h
 */

#define GLOBAL_CMDS "hVv"

#define GLOBAL_OPTS bool verbose

#define GLOBAL_OPTS_INIT .verbose = false

#define GLOBAL_CMDS_GETOPT \
case 'h': \
	cmd_help(); \
	break; \
\
case 'V': \
	CMD_VERSION(NAME, DESCR); \
	break; \
\
case 'v': \
	opts.verbose = true; \
	break; \

#define GLOBAL_HELP "    -h            Display this help message\n" \
	"    -V            Display vserver-utils version\n" \
	"    -v            Enable verbose output\n"

#define DEFAULT_GETOPT \
default: \
	printf("Try '%s -h' for more information\n", argv[0]); \
	exit(1); \
	break; \


#define CMD_VERSION(name, desc) do { \
	printf("%s -- %s\n", name, desc); \
	printf("This program is part of %s\n\n", PACKAGE_STRING); \
	printf("Copyright (c) 2005 The vserver-utils Team\n"); \
	printf("This program is free software; you can redistribute it and/or\n"); \
	printf("modify it under the terms of the GNU General Public License\n"); \
	exit(0); \
}	while(0)

#define VPRINTF(opts, fmt, ...) if ((opts)->verbose) printf(fmt , __VA_ARGS__ )
#ifdef DEBUG
#define DEBUGF(fmt, ...) vu_printf("DEBUG: " fmt , __VA_ARGS__ )
#else
#define DEBUGF(fmt, ...)
#endif

