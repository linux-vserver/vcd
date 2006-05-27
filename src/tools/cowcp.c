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
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <vserver.h>

#include "printf.h"
#include "tools.h"
#include "vlist.h"

#define NAME  "cowcp"
#define DESCR "Copy utility using CoW"

#define SHORT_OPTS "rf"

struct options {
	GLOBAL_OPTS;
	bool recurse;
	bool force;
};

/* init program data */
struct options opts = {
	GLOBAL_OPTS_INIT,
	.recurse = false,
	.force   = false,
};

static inline
void cmd_help()
{
	vu_printf("Usage: %s <opts>* <src> <dst>\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -f            Force overwriting existing files\n"
	       "    -r            Process subdirectories recursively\n"
	       "\n"
	       NAME);
	exit(EXIT_SUCCESS);
}

static
bool confirm_del(mode_t st_mode, const char *dst) {
	char c;
	bool r = false;
	if (S_ISDIR(st_mode)) {
		vu_printf("Remove directory %s? [N/y]: ", dst);
	} else if (S_ISREG(st_mode)) {
		vu_printf("Remove regular file %s? [N/y]: ", dst);
	} else if (S_ISBLK(st_mode)) {
		vu_printf("Remove block dev %s? [N/y]: ", dst);
	} else if (S_ISCHR(st_mode)) {
		vu_printf("Remove char dev %s? [N/y]: ", dst);
	} else if (S_ISLNK(st_mode)) {
		vu_printf("Remove symlink %s? [N/y]: ", dst);
	} else if (S_ISFIFO(st_mode)) {
		vu_printf("Remove fifo %s? [N/y]: ", dst);
	} else if (S_ISSOCK(st_mode)) {
		vu_printf("Remove socket %s? [N/y]: ", dst);
	} else
		return false;
	while (true) {
		switch (read(0, &c, 1)) {
			case -1:
				if (errno != EAGAIN && errno != EINTR)
					return false;
				break;
			case 0:
				return false;
			case 1:
				if (c == '\n')
					return r;
				else
					r = (c == 'y' || c == 'Y');
			default:
				return false;
		}
	}
}

static
int cp_dir(const struct stat *s_st, const char *dst)
{
	int r = errno = 0;
	if (mkdir(dst, 0) == -1) {
		vu_printf("mkdir(%s): %s\n", dst, strerror(r = errno));
		goto out;
	}
	if (chown(dst, s_st->st_uid, s_st->st_gid) == -1) {
		vu_printf("chown(%s): %s\n", dst, strerror(r = errno));
		goto out;
	}
	if (chmod(dst, s_st->st_mode) == -1) {
		vu_printf("chmod(%s): %s\n", dst, strerror(r = errno));
		goto out;
	}
out:
	return (errno = r) == 0 ? 0 : -1;
}

static
int cp_cow(const char *src, const char *dst)
{
	/* init syscall data */
	struct vx_iattr iattr = {
		.filename = NULL,
		.flags = IATTR_IMMUTABLE | IATTR_IUNLINK,
		.mask  = IATTR_IMMUTABLE | IATTR_IUNLINK,
	};
	if (link(src, dst) == -1) {
		if (errno == EPERM) {
			// Make sure src is CoW, not just IMMUTABLE
			iattr.filename = src;
			if (vx_set_iattr(&iattr) == 0 && link(src, dst) == 0)
				goto ok;
		}
		vu_printf("link(%s, %s): %s\n", src, dst, strerror(errno));
		return -1;
	}
ok:
	iattr.filename = dst;
	if (vx_set_iattr(&iattr) == -1) {
		vu_printf("iattr(%s): %s\n", dst, strerror(errno));
		return -1;
	}
	return 0;
}

static
int process_file(const char *src, const char *dst)
{
	struct stat s_st;
	struct stat d_st;
	if (lstat(src, &s_st) == -1) {
		vu_printf("lstat(%s): %s\n", src, strerror(errno));
		return -1;
	}

	/* Check it target exists */
	if (lstat(dst, &d_st) == -1) {
		if (errno != ENOENT) {
			vu_printf("lstat(%s): %s\n", dst, strerror(errno));
			return -1;
		}
	} else {
		if (S_ISDIR(d_st.st_mode)) {
			goto dir;
		} else {
			/* Try removing existing object */
			if (opts.force || confirm_del(d_st.st_mode, dst)) {
				if (unlink(dst) == -1) {
					vu_printf("unlink(%s): %s\n", dst, strerror(errno));
					return -1;
				}
			} else {
				vu_printf("cp(%s): target untouched\n", dst);
				return -1;
			}
		}
	}

	/* Do our copy if we know how to do */
	if (S_ISDIR(s_st.st_mode)) {
		if (cp_dir(&s_st, dst) == -1)
			return -1;
		else
			goto dir;
	} else if (S_ISREG(s_st.st_mode)) {
		return cp_cow(src, dst);
	} else if (S_ISBLK(s_st.st_mode)) {
		vu_printf("cp(%s): block devices not supported\n", src);
	} else if (S_ISCHR(s_st.st_mode)) {
		vu_printf("cp(%s): char devices not supported\n", src);
	} else if (S_ISLNK(s_st.st_mode)) {
		vu_printf("cp(%s): symbolic links not supported\n", src);
	} else if (S_ISFIFO(s_st.st_mode)) {
		vu_printf("cp(%s): fifos not supported\n", src);
	} else if (S_ISSOCK(s_st.st_mode)) {
		vu_printf("cp(%s): sockets not supported\n", src);
	}
	return -1;
dir:
	/* Jump into the directory */
	if (opts.recurse) {
		char* n_src = NULL;
		char* n_dst = NULL;
		int r = 0;
		int l_src = strlen(src);
		int l_dst = strlen(dst);
		DIR *dir = opendir(src);
		struct dirent *ent;

		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_name && !(ent->d_name[0] == '.' && (ent->d_name[1] == '\0' || (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))) {
				int l = strlen(ent->d_name);
				char * tmp;
				if ((tmp = realloc(n_src, l_src + l + 2))) {
					strcpy(tmp, src);
					strcpy(tmp + l_src + 1, ent->d_name);
					tmp[l_src] = '/';
					tmp[l_src + l + 1] = '\0';
					n_src = tmp;
				} else {
					r = -1;
					break;
				}
				if ((tmp = realloc(n_dst, l_dst + l + 2))) {
					strcpy(tmp, dst);
					strcpy(tmp + l_dst + 1, ent->d_name);
					tmp[l_dst] = '/';
					tmp[l_dst + l + 1] = '\0';
					n_dst = tmp;
				} else {
					r = -1;
					break;
				}
				if (process_file(n_src, n_dst) != 0)
					r = -1;
			}
		}
		closedir(dir);
		free(n_src);
		free(n_dst);
		return r == 0 ? 0 : -1;
	} else
		return 0;
}

int main(int argc, char *argv[])
{
	int c;
	int errcnt = 0;
	
	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'r':
				opts.recurse = true;
				break;

			case 'f':
				opts.force = true;
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	/* check file argument */
	if (argc != optind + 2) {
		vu_printf("Expecting exactly 1 source and 2 target\n");
		errcnt = 1;
	} else
		errcnt = process_file(argv[optind], argv[optind+1]);
	
	if (errcnt == 0)
		exit(EXIT_SUCCESS);
	else
		exit(EXIT_FAILURE);
}
