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

#include <lucid/printf.h>
#include "tools.h"
#include "vlist.h"

#define NAME  "vunify"
#define DESCR "VPS unification tool"

#define SHORT_OPTS "acrx:pd"
#define BUFFSZ 8096

struct options {
	GLOBAL_OPTS;
	bool ask;
	bool recurse;
	bool pretend;
	bool diffs;
	bool cow;
	xid_t xid;
	char *sfile;
};

/* init program data */
struct options opts = {
	GLOBAL_OPTS_INIT,
	.ask     = false,
	.recurse = false,
	.pretend = false,
	.diffs   = false,
	.cow     = false,
	.xid     = 0,
	.sfile   = 0,
};

static inline
void cmd_help()
{
	_lucid_printf("Usage: %s <opts>* <sfile> <dfile>*\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -a            Ask before performing unification\n"
	       "    -d            Show different files\n"
	       "    -p            Pretend performing unification\n"
	       "    -r            Process subdirectories recursively\n"
	       "    -c            Unify with Copy on Write\n"
	       "    -x <xid>      Context ID\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

static
int unify(const char *base, const char *dest, bool cow)
{
	int l = strlen(dest);
	char *buf = malloc(l + 15);
	int my_err = 0;
	_lucid_snprintf(buf, l + 15, "%s.%d", dest, getpid());
	if (rename(dest, buf) != 0) {
		_lucid_dprintf(2, "Failed to unlink '%s': %s\n", dest, strerror(my_err = errno));
		goto out;
	}
	if (link(base, dest) != 0) {
		if (errno == EPERM) {
			// Check if base is IMMUTABLE...
			struct vx_iattr iattr = {
				.filename = base,
				.flags = IATTR_IMMUTABLE | IATTR_IUNLINK,
				.mask  = IATTR_IMMUTABLE | IATTR_IUNLINK,
			};
			if (vx_set_iattr(&iattr) == 0 && link(base, dest) == 0)
				goto ok;
		}
		_lucid_dprintf(2, "Failed to hardlink '%s' to '%s': %s\n", base, dest, strerror(my_err = errno));
		rename(buf, dest); // Restore our file
		goto out;
	}
ok:
	if (unlink(buf) != 0) {
		_lucid_dprintf(2, "Failed to unlink '%s': %s\n", dest, strerror(my_err = errno));
		goto out;
	}

	/* init syscall data */
	struct vx_iattr iattr = {
		.filename = base,
		.flags = IATTR_IMMUTABLE | (cow ? IATTR_IUNLINK : 0),
		.mask  = IATTR_IMMUTABLE | IATTR_IUNLINK,
	};
	if (vx_set_iattr(&iattr) != 0) {
		_lucid_dprintf(2, "Failed to set IMMUTABLE on '%s': %s\n", base, strerror(my_err = errno));
		goto out;
	}
out:
	free(buf);
	errno = my_err;
	return my_err != 0;
}

static
int can_unify(const char *base, const char *dest)
{
	struct stat stb;
	struct stat std;

	if (lstat(base, &stb)) {
		_lucid_dprintf(2, "Cannot stat '%s': %s\n", base, strerror(errno));
		return 0;
	}
	if (lstat(dest, &std)) {
		VPRINTF(&opts, "Cannot stat '%s': %s\n", base, strerror(errno));
		return 0;
	}
	if (!S_ISREG(stb.st_mode) || !S_ISREG(std.st_mode)) {
		VPRINTF(&opts, "'%s' and '%s' are not both regular files.\n", base, dest);
		return 0;
	}
	if (stb.st_ino == std.st_ino) {
		VPRINTF(&opts, "'%s' and '%s' are already unified or hardlinked.\n", base, dest);
		return 0;
	}
	if (stb.st_dev != std.st_dev) {
		VPRINTF(&opts, "'%s' and '%s' are not on same filesystem.\n", base, dest);
		return 0;
	}
	if (stb.st_size != std.st_size) {
		if (opts.diffs)
			_lucid_printf("Not unifying '%s', size different\n", dest);
		return 0;
	}
	if (stb.st_uid != std.st_uid || stb.st_gid != std.st_gid || stb.st_mode != std.st_mode) {
		if (opts.diffs)
			_lucid_printf("Not unifying '%s', ownership/mode different\n", dest);
		return 0;
	}

	int fd_b = -1;
	int fd_d = -1;
	if ((fd_b = open(base, O_RDONLY)) == -1) {
		_lucid_dprintf(2, "Failed to open '%s': %s\n", base, strerror(errno));
		return 0;
	}
	if ((fd_d = open(dest, O_RDONLY)) == -1) {
		_lucid_dprintf(2, "Failed to open '%s': %s\n", dest, strerror(errno));
		close(fd_b);
		return 0;
	}
	char buf_b[BUFFSZ];
	char buf_d[BUFFSZ];
	int nb = 0;
	int nd = 0;
	while (true) {
		if ((nb = read(fd_b, buf_b, sizeof(buf_b))) == -1)
			_lucid_dprintf(2, "Read on base failed: %s\n", strerror(errno));
		if ((nd = read(fd_d, buf_d, sizeof(buf_d))) == -1)
			_lucid_dprintf(2, "Read on dest failed: %s\n", strerror(errno));
		if (nb != nd || nb == 0 || nd == 0 || nb == -1 || nd == -1)
			break;
		if (memcmp(buf_b, buf_d, nb) != 0) {
			nb = -1;
			VPRINTF(&opts, "Found a diff in %s.\n", base + strlen(opts.sfile) + 1);
			break;
		}
	}
	close(fd_b);
	close(fd_d);
	if (nb != nd && opts.diffs)
		_lucid_printf("Not unifying '%s', content different\n", dest);
	return nb == nd && nb == 0;
}

static
int ask_unify(char *base, char *dest)
{
	_lucid_printf("Unify '%s' and '%s'? [y/N]: ", base, dest);
	int c = getchar();
	int tmp;
	while (c != '\n' && (tmp = getchar()) != EOF && tmp != '\n');

	return c == 'y' || c == 'Y';
}

static
int can_unify_dir(const char *base, const char *dest)
{
	struct stat stb;
	struct stat std;

	if (lstat(base, &stb)) {
		_lucid_dprintf(2, "Cannot stat '%s': %s\n", base, strerror(errno));
		return 0;
	}
	if (lstat(dest, &std)) {
		VPRINTF(&opts, "Cannot stat '%s': %s\n", base, strerror(errno));
		return 0;
	}
	if (!S_ISDIR(stb.st_mode) || !S_ISDIR(std.st_mode)) {
		VPRINTF(&opts, "'%s' and '%s' are not both directories.\n", base, dest);
		return 0;
	}
	if (stb.st_ino == std.st_ino) {
		VPRINTF(&opts, "'%s' and '%s' are same directory or hardlinked.\n", base, dest);
		return 0;
	}
	if (stb.st_dev != std.st_dev) {
		VPRINTF(&opts, "'%s' and '%s' are not on same filesystem.\n", base, dest);
		return 0;
	}
}

static
int walk_tree(char *base, char *dest)
{
	DIR *dir = opendir(base);
	char *nbase = NULL;
	int blen = strlen(base);
	int dlen = strlen(dest);
	char *ndest = NULL;
	int nufiles = 0;

	if (dir == 0) {
		_lucid_dprintf(2, "Failed to open dir '%s': %s\n", base, strerror(errno));
		return 0;
	}

	struct dirent *ent;
	struct stat sb;

	if (!can_unify_dir(base, dest))
		return 0;

	VPRINTF(&opts, "Processing directory %s ...\n", base);

	while ((ent = readdir(dir)) != 0) {
		if (ent->d_name[0] == '.' && (ent->d_name[1] == '\0' || (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
			continue; // Skip . and .. entries
		if (strcmp(ent->d_name, ".keep") == 0)
			continue; // Skip Gentoo .keep files
		int l = strlen(ent->d_name);
		char *tmp;
		// Build new base filename (fully qualified path)
		if ((tmp = realloc(nbase, sizeof(char) * (blen + l + 2))) == NULL)
			continue;
		nbase = tmp;
		strcpy(nbase, base);
		strcpy(nbase + blen, "/");
		strcpy(nbase + blen + 1, ent->d_name);

		if (lstat(nbase, &sb) == -1) {
			_lucid_dprintf(2, "Failed to stat '%s': %s\n", ent->d_name, strerror(errno));
			continue;
		}
		// Build new destination filename (fully qualified path)
		if ((tmp = realloc(ndest, sizeof(char) * (dlen + l + 2))) == NULL)
			continue;
		ndest = tmp;
		strcpy(ndest, dest);
		strcpy(ndest + dlen, "/");
		strcpy(ndest + dlen + 1, ent->d_name);

		if (S_ISDIR(sb.st_mode) && opts.recurse) {
			nufiles += walk_tree(nbase, ndest);
		} else if (S_ISREG(sb.st_mode)) {
			if (can_unify(nbase, ndest)) {
				if (opts.pretend) {
					_lucid_printf("Would unify '%s'.\n", ndest);
					nufiles++;
				} else {
					if (opts.ask && !ask_unify(nbase, ndest))
						continue;
					if (unify(nbase, ndest, opts.cow) == 0)
						nufiles++;
				}
			}
		}
	}
	free(nbase);
	free(ndest);
	closedir(dir);
	return nufiles;
}

int main(int argc, char *argv[])
{
	int c;
	int nufiles = 0;

	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;

		switch (c) {
			GLOBAL_CMDS_GETOPT

			case 'a':
				opts.ask = true;
				break;

			case 'p':
				opts.pretend = true;
				break;

			case 'c':
				opts.cow = true;
				break;

			case 'r':
				opts.recurse = true;
				break;

			case 'd':
				opts.diffs = true;
				break;

			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;

			DEFAULT_GETOPT
		}
	}
	/* check file argument */
	if (argc <= optind) {
		_lucid_dprintf(2, "Missing reference dir ...\nTry %s -h for help\n", argv[0]);
		exit(EXIT_FAILURE);
	} else {
		opts.sfile = argv[optind++];
	}

	if (argc == optind)
		nufiles = walk_tree(opts.sfile, ".");
	else for (int i = optind; i < argc; ++i)
		nufiles += walk_tree(opts.sfile, argv[i]);

	VPRINTF(&opts, "%d files have been unified\n", nufiles);
	exit(EXIT_SUCCESS);
}
