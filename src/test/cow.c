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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utime.h>
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <vserver.h>

#include "test.h"

#define NAME  "cow"
#define DESCR "Test program for VServer CoW"

#define SHORT_OPTS "uin:"

struct options {
	GLOBAL_OPTS;
	bool immutable;
	bool iunlink;
	int fcnt;
};

struct options opts = {
	GLOBAL_OPTS_INIT,
	.immutable = false,
	.iunlink = false,
};

static inline
void cmd_help()
{
	printf("Usage: %s <opts>* <file>*\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -n <int>      Count of unified files (1-9)\n"
	       "    -i            Test with IMMUTABLE attr\n"
	       "    -u            Test with IUNLINK attr\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

static
int set_iattr(char* file, bool immutable, bool iunlink)
{
	/* init syscall data */
	struct vx_iattr iattr = {
		.filename = file,
		.flags = (immutable ? IATTR_IMMUTABLE : 0) |
			(iunlink ? IATTR_IUNLINK : 0),
		.mask  = IATTR_IMMUTABLE | IATTR_IUNLINK,
	};
	return vx_set_iattr(&iattr);
}

static
int get_iattr(char* file, bool* immutable, bool* iunlink, nlink_t* lcnt)
{
	struct stat sb;
	if (lstat(file, &sb) == -1 && errno == ENOENT)
		return -1;
	if (lcnt)
		*lcnt = sb.st_nlink;

	/* init syscall data */
	struct vx_iattr iattr;
	iattr.filename = file;

	/* init list output chars */
	const char marker[9] = "xawhfbui";
	const uint32_t attrs[9] = {
		IATTR_TAG, IATTR_ADMIN, IATTR_WATCH, IATTR_HIDE,
		IATTR_FLAGS, IATTR_BARRIER, IATTR_IUNLINK,
		IATTR_IMMUTABLE, 0
	};

	char buf[17];
	char *ptr = buf;
	int rc = 0;

	memset(buf, '\0', sizeof(buf));

	if (S_ISREG(sb.st_mode))
		*ptr = '-';
	if (S_ISDIR(sb.st_mode))
		*ptr = 'd';
	if (S_ISCHR(sb.st_mode))
		*ptr = 'c';
	if (S_ISBLK(sb.st_mode))
		*ptr = 'b';
	if (S_ISFIFO(sb.st_mode))
		*ptr = 'f';
	if (S_ISLNK(sb.st_mode))
		*ptr = 'l';
	if (S_ISSOCK(sb.st_mode))
		*ptr = 's';
	ptr++;

	/* syscall */
	if (vx_get_iattr(&iattr) == -1) {
		rc = errno;
		memcpy(ptr, "- ERR - ", 8);
		ptr += 8;
	} else {
		int i;
		if (immutable)
			*immutable = (iattr.mask & IATTR_IMMUTABLE) ? ((iattr.mask & IATTR_IMMUTABLE) ? true : false) : false;
		if (iunlink)
			*iunlink = (iattr.mask & IATTR_IUNLINK) ? ((iattr.mask & IATTR_IUNLINK) ? true : false) : false;

		for (i = 0; marker[i]; i++) {
			if (iattr.mask & attrs[i])
				if (iattr.flags & attrs[i])
					*ptr++ = toupper(marker[i]);
				else
					*ptr++ = marker[i];
			else
				*ptr++ = '-';
		}
	}

	if (iattr.mask & IATTR_TAG)
		snprintf(ptr, 7, " %5d", iattr.xid);
	else
		snprintf(ptr, 7, " noxid");

	printf("  %s   [%.2d]   %s\n", buf, sb.st_nlink, file);
	errno = rc;
	return rc != 0 ? -1 : 0;
}

/*
 * Prepare for testing (setup test-files)
 */
void test_init(int fcnt, bool immutable, bool iunlink)
{
	int f;
	report("open O_CREAT|O_WRONLY", "CoW_0", f = open("CoW_0", O_CREAT | O_WRONLY));
	if (f != -1) {
		vreport(&opts, "write \"CoW_0\\n\"", "CoW_0", write(f, "CoW_0\n", 6));
		int i;
		char buf[6];
		for (i = 1; i < fcnt && i < 10; i++) {
			snprintf(buf, 6, "CoW_%d", i);
			report("link CoW_0 CoW_x", buf, link("CoW_0", buf));
		}
		vreport(&opts, "close", "CoW_0", close(f));
		vreport(&opts, "set_iattr()", "CoW_0", set_iattr("CoW_0", immutable, iunlink));
	}
}

/*
 * Cleanup from testing (remove test-files)
 */
void test_cleanup(int fcnt)
{
	VPRINTF(&opts, "---- Cleaning up (%d files)\n", fcnt);
	char buf[6];
	for (int i = 0; i < fcnt && i < 10; i++) {
		snprintf(buf, 6, "CoW_%d", i);
		vreport(&opts, "set_iattr(iu)", buf, set_iattr(buf, false, false));
		vreport(&opts, "unlink", buf, unlink(buf));
	}
}

void test_check(int fcnt, bool  immutable, bool iunlink)
{
	char buf[6];
	for (int i = 0; i < fcnt && i < 10; i++) {
		snprintf(buf, 6, "CoW_%d", i);
		if (get_iattr(buf, NULL, NULL, NULL) == -1)
			printf("  N/A                      %s\n", buf);
	}
	/* TODO: check for $X? files */
	DIR* d = opendir(".");
	if (d) {
		struct dirent *de;
		while (de = readdir(d)) {
			if (strncmp(de->d_name, buf, 4) == 0 && strlen(de->d_name) == 6) {
				printf("[W]  Unexpected file '%.5s\\%o' found\n", de->d_name, (uint8_t)de->d_name[5]);
				unlink(de->d_name);
			}
		}
		closedir(d);
	}
}

void test_unlink(int fcnt)
{
	char buf[6];
	snprintf(buf, 6, "CoW_%d", fcnt > 1 ? 1 : 0);
	report("unlink", buf, unlink(buf));
}

void test_openrw(int fcnt)
{
	int f;
	char buf[6];
	snprintf(buf, 6, "CoW_%d", fcnt > 1 ? 1 : 0);
	report("open O_WRONLY", buf, f = open(buf, O_WRONLY));
	if (f != -1) {
		vreport(&opts, "write \"CoW_x\\n\"", buf, write(f, buf, 5));
		vreport(&opts, "close", buf, close(f));
	}
}

void test_chown(int fcnt)
{
	char buf[6];
	snprintf(buf, 6, "CoW_%d", fcnt > 1 ? 1 : 0);
	report("chown 1 1", buf, chown(buf, 1, 1));
}

void test_chmod(int fcnt)
{
	char buf[6];
	snprintf(buf, 6, "CoW_%d", fcnt > 1 ? 1 : 0);
	report("chown 0444", buf, chmod(buf, 0444));
}

void test_touch(int fcnt)
{
	char buf[6];
	snprintf(buf, 6, "CoW_%d", fcnt > 1 ? 1 : 0);
	report("touch", buf, utime(buf, NULL));
}

int main(int argc, char *argv[])
{
	char c;
	/* init program data */

	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'u':
				opts.iunlink = true;
				break;
			
			case 'i':
				opts.immutable = true;
				break;
			
			case 'n':
				opts.fcnt = atoi(optarg);
				break;
				
			DEFAULT_GETOPT
		}
	}
	if (opts.fcnt < 1)
		opts.fcnt = 1;
	else if (opts.fcnt > 9)
		opts.fcnt = 9;

#define DOTEST(n, im, iu, title, op) \
	printf("========= %c%c %s\n", (im ? 'I' : 'i'), (iu ? 'U' : 'u'), title); \
	test_init(n, im, iu); \
	test_ ## op (n); \
	test_check (n, im, iu); \
	test_cleanup(n);

	DOTEST(opts.fcnt, opts.immutable, opts.iunlink, "chmod", chmod)
	DOTEST(opts.fcnt, opts.immutable, opts.iunlink, "chown", chown)
	DOTEST(opts.fcnt, opts.immutable, opts.iunlink, "write", openrw)
	DOTEST(opts.fcnt, opts.immutable, opts.iunlink, "touch", touch)
	DOTEST(opts.fcnt, opts.immutable, opts.iunlink, "unlink", unlink)
}
