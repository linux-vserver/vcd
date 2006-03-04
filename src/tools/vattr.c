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

#define NAME  "vattr"
#define DESCR "File Attribute Manager"

#define SHORT_OPTS "SGacdf:lrx:"

typedef enum { VATTR_NONE, VATTR_GET, VATTR_SET } command_t;

struct options {
	GLOBAL_OPTS;
	command_t cmd;
	bool all;
	bool cross;
	bool dirent;
	list_t *flags;
	bool follow;
	bool recurse;
	xid_t xid;
	char *file;
};

/* init program data */
struct options opts = {
	GLOBAL_OPTS_INIT,
	.cmd     = VATTR_NONE,
	.all     = false,
	.cross   = false,
	.dirent  = false,
	.flags   = 0,
	.follow  = false,
	.recurse = false,
	.xid     = 0,
	.file    = 0,
};

struct stat st;

static inline
void cmd_help()
{
	vu_printf("Usage: %s <opts>* <file>*\n"
	       "\n"
	       "Available commands:\n"
	       "    -S            Set file attributes\n"
	       "    -G            Get file attributes\n"
	       "\n"
	       "Available options:\n"
	       GLOBAL_HELP
	       "    -a            Process entries starting with a dot\n"
	       "    -c            Do not cross filesystems\n"
	       "    -d            Process directory entries instead of contents\n"
	       "    -f <flags>    Set flags described in <flags>\n"
	       "    -l            Follow symbolic links\n"
	       "    -r            Process subdirectories recursively\n"
	       "    -x <xid>      Context ID\n"
	       "\n"
	       "Flag list format string:\n"
	       "    <flags> = [~]<flag>,[~]<flag>,...\n"
	       "\n"
	       "    <flag> is one of: XID, ADMIN, WATCH, HIDE, FLAGS,\n"
	       "                      BARRIER, IUNLINK, IMMUTABLE\n"
	       "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

static
int do_iattr(char *file, struct stat *sb, char *display, char *src)
{
	int rc = 0;
	
	/* init syscall data */
	struct vx_iattr iattr;
	
	/* init attribute lists */
	list_t *ip = iattr_list_init();
	
	/* set filename */
	iattr.filename = file;
	
	if (opts.cmd == VATTR_SET) {
		const char clmod = '~'; // clear flag modifier
		
		if (opts.flags == 0)
			goto set_iattr;
		
		/* flag list handling */
		list_link_t link = {
			.p = ip,
			.d = opts.flags,
		};
		
		/* validate descending list */
		if (list_validate_flag(&link, clmod) == -1) {
			perror("List validation failed");
			return 1;
		}
		
		/* we need dummy flags and mask to convert them to uint32_t later */
		uint64_t flags = 0, mask = 0;
		
		/* convert given descending list to flags using the pristine copy */
		list_list2flags(&link, clmod, &flags, &mask);
		
		iattr.flags = (uint32_t)flags;
		iattr.mask  = (uint32_t)mask;
		
set_iattr:
		/* set xid */
		iattr.xid = opts.xid;
		
		if (opts.xid != 0) {
			iattr.flags |= IATTR_TAG;
			iattr.mask  |= IATTR_TAG;
		}
		
		/* syscall */
		if (vx_set_iattr(&iattr) == -1) {
			vu_printf("vx_set_iattr(%s): %s\n", iattr.filename, strerror(errno));
			return 1;
		}
	} else {
		/* init list output chars */
		const char marker[9] = "xawhfbui";
		
		char buf[17];
		char *ptr = buf;
		
		memset(buf, ' ', sizeof buf);
		
		if (S_ISREG(sb->st_mode))
			*ptr = '-';
		if (S_ISDIR(sb->st_mode))
			*ptr = 'd';
		if (S_ISCHR(sb->st_mode))
			*ptr = 'c';
		if (S_ISBLK(sb->st_mode))
			*ptr = 'b';
		if (S_ISFIFO(sb->st_mode))
			*ptr = 'f';
		if (S_ISLNK(sb->st_mode))
			*ptr = 'l';
		if (S_ISSOCK(sb->st_mode))
			*ptr = 's';
		
		ptr++;
		
		/* syscall */
		if (vx_get_iattr(&iattr) == -1) {
			memcpy(ptr, "- ERR - ", 8);
			ptr += 8;
			rc = 1;
		} else {
			/* print pretty flag list */
			list_foreach(ip, i) {
				uint64_t data = *(uint64_t *)(ip->node+i)->data;
				
				if (!(iattr.mask & data))
					*ptr++ = '-';
				else if (iattr.flags & data)
					*ptr++ = toupper(marker[i]);
				else
					*ptr++ = marker[i];
			}
		}
		
		if (iattr.mask & IATTR_TAG)
			vu_snprintf(ptr, 7, " %5d", iattr.xid);
		else
			vu_snprintf(ptr, 7, " noxid");
		
		vu_printf("%s", buf);
		
		if (src != 0)
			vu_printf(" %s ->", src);
		
		if (display != 0)
			vu_printf(" %s\n", display);
		else
			vu_printf(" %s\n", file);
	}
	
	return rc;
}

#define S_ISXDEV(SB) (st.st_dev != SB.st_dev)
#define F_ISDOT(FB)  (FB[0] == '.')

static
int handle_file(char *file, char *display)
{
	struct stat sb;
	
	if (F_ISDOT(file) && !opts.all)
		goto out;
	
	if (lstat(file, &sb) == -1) {
		vu_printf("lstat(%s): %s\n", file, strerror(errno));
		return 1;
	}
	
	if (S_ISXDEV(sb) && opts.cross)
		goto out;
	
	if (S_ISLNK(sb.st_mode)) {
		char link_buf[PATH_MAX];
		int link_len;
		
		if ((link_len = readlink(file, link_buf, PATH_MAX)) == -1) {
			/* skip kernel processes in /proc which
			   pass stat, but link cannot be read */
			if (errno == ENOENT)
				return 0;
			
			vu_printf("readlink(%s): %s\n", display, strerror(errno));
			return 1;
		}
		
		link_buf[link_len] = '\0';
		
		if (opts.follow) {
			if (stat(link_buf, &sb) == -1) {
				vu_printf("stat(%s): %s\n", link_buf, strerror(errno));
				return 1;
			}
			
			return do_iattr(link_buf, &sb, display, 0);
		} else {
			return do_iattr(file, &sb, link_buf, display);
		}
	} else {
		return do_iattr(file, &sb, display, 0);
	}
	
out:
	return 0;
}

static
int walk_tree(char *path, char *root)
{
	int errcnt      = 0;
	size_t path_len = strlen(path);
	DIR *dir        = opendir(path);
	
	if (dir == 0) {
		vu_printf("opendir(%s): %s\n", path, strerror(errno));
		return 1;
	}
	
	/* show current dir (only if we're not listing our root) */
	if (root != 0)
		errcnt += handle_file(path, root);
	
	/* strip trailing slash */
	while (path_len > 0 && path[path_len-1] == '/')
		--path_len;
	
	struct dirent *ent;
	struct stat sb;
	
	int cwd = open(".", O_RDONLY);
	chdir(path);
	
	while ((ent = readdir(dir)) != 0) {
		if (lstat(ent->d_name, &sb) == -1) {
			vu_printf("lstat(%s): %s\n", ent->d_name, strerror(errno));
			return errcnt + 1;
		}
		
		char new_root[PATH_MAX];
		char *ptr = new_root;
		
		if (strcmp(path, ".") != 0) {
			if (root == 0) {
				memcpy(ptr, path, path_len);
				ptr += path_len;
			} else {
				memcpy(ptr, root, strlen(root));
				ptr += strlen(root);
			}
			
			memcpy(ptr++, "/", 1);
		}
		
		memcpy(ptr, ent->d_name, strlen(ent->d_name));
		ptr += strlen(ent->d_name);
		
		*ptr = '\0';
		
		if (S_ISDIR(sb.st_mode) && !F_ISDOT(ent->d_name) && opts.recurse) {
			errcnt += walk_tree(ent->d_name, new_root);
			continue;
		}
		
		errcnt += handle_file(ent->d_name, new_root);
	}
	
	fchdir(cwd);
	close(cwd);
	
	closedir(dir);
	
	return errcnt;
}

#undef S_ISXDEV
#undef S_ISDOT

static
int process_file(char *path)
{
	int cwd = open(".", O_RDONLY);
	int rc = 0;
	
	if (lstat(path, &st) == -1) {
		vu_printf("lstat(%s): %s\n", path, strerror(errno));
		return 1;
	}

	if (opts.dirent || !S_ISDIR(st.st_mode || opts.cmd == VATTR_SET)) {
		/* First operate on the file/folder itself if either:
		 * - It's not a directory
		 * - Even dires should be processed
		 * - If we are setting value
		 */
		char *pathc = strdup(path);
		char *basec = strdup(path);
		char *dname = dirname(pathc);
		char *bname = basename(basec);
		
		chdir(dname);
		rc = handle_file(bname, path);
	}

	// If we need to recurse, do it
	if (S_ISDIR(st.st_mode) &&
	   ((opts.cmd == VATTR_SET && opts.recurse) || (!opts.cmd == VATTR_GET && !opts.dirent))) {
		rc = walk_tree(path, 0);
	}
	
	fchdir(cwd);
	close(cwd);
	
	return rc;
}

int main(int argc, char *argv[])
{
	int c;
	int errcnt = 0;
	const char delim = ','; // list delimiter
	
	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'S':
				if (opts.cmd != VATTR_NONE)
					cmd_help();
				else
					opts.cmd = VATTR_SET;
				break;
			
			case 'G':
				if (opts.cmd != VATTR_NONE)
					cmd_help();
				else
					opts.cmd = VATTR_GET;
				break;
			
			case 'a':
				opts.all = true;
				break;
			
			case 'c':
				opts.cross = true;
				break;
			
			case 'd':
				opts.dirent = true;
				break;
			
			case 'f':
				opts.flags = list_parse_list(optarg, delim);
				break;
			
			case 'l':
				opts.follow = true;
				break;
			
			case 'r':
				opts.recurse = true;
				break;
			
			case 'x':
				opts.xid = (xid_t) atoi(optarg);
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	/* check file argument */
	if (argc == optind)
		errcnt = process_file(".");
	else for(int i = optind; i < argc; ++i)
		errcnt += process_file(argv[i]);
	
	if (errcnt == 0)
		exit(EXIT_SUCCESS);
	else
		exit(EXIT_FAILURE);
}
