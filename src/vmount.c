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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <vserver.h>

#include "tools.h"

#define NAME  "vmount"
#define DESCR "Filesystem Manager"

#define SHORT_OPTS "MUf:m:r:n"

/* sys/mount.h does not define MS_REC! */
#ifndef MS_REC
#define MS_REC 16384
#endif

/* sys/mount.h does not set MNT_DETACH! */
#ifndef MNT_DETACH
#define MNT_DETACH 0x00000002
#endif

struct commands {
	bool mount;
	bool umount;
};

struct options {
	char *fstab;
	char *mtab;
	char *rbind;
	bool nomtab;
};

struct mntspec {
	char *source;
	char *target;
	char *vfstype;
	unsigned long flags;
	char *data;
};

int cwd_fd;
int root_fd;

static inline
void cmd_help()
{
	printf("Usage: %s <command> <opts>* -- <program> <args>*\n"
	            "\n"
	            "Available commands:\n"
	            "    -M            Mount vserver filesystems\n"
	            "    -U            Unmount vserver filesystems\n"
	            "\n"
	            "Available options:\n"
	            "    -f <file>     Use fstab <file>\n"
	            "    -m <file>     Use mtab <file>\n"
	            "    -r <path>     Remount <path> to /\n"
	            "    -n            Do not update mtab\n"
	            "\n",
	       NAME);
	exit(EXIT_SUCCESS);
}

static inline
void restore_root(void)
{
	fchdir(root_fd);
	chroot(".");
}

static
int secure_chdir(char *target)
{
	int target_fd = 0;
	
	if (fchdir(cwd_fd) == -1)
		goto error;
	
	if (chroot(".") == -1)
		goto error;
	
	if (chdir(target) == -1)
		goto error;
	
	target_fd = open(".", O_RDONLY|O_DIRECTORY);
	
	if (target_fd == -1)
		goto error;
	
	restore_root();
	
	if (fchdir(target_fd) == -1)
		goto error;
	
	close(target_fd);
	return 0;
	
error:
	close(target_fd);
	return -1;
}

static
int update_mtab(struct mntspec *fsent, struct options *opts)
{
	int mtab_fd = 0;
	char *line;
	
	if (fchdir(cwd_fd) == -1)
		goto error;
	
	if (chroot(".") == -1)
		goto error;
	
	mtab_fd = open(opts->mtab, O_CREAT|O_APPEND|O_WRONLY, 0644);
	
	if (mtab_fd == -1)
		goto error;
	
	if (flock(mtab_fd, LOCK_EX) == -1)
		goto error;
	
	if (fsent->vfstype == 0)
		fsent->vfstype = "none";
	
	if (fsent->data == 0)
		fsent->data = "defaults";
	
	asprintf(&line, "%s %s %s %s 0 0\n", fsent->source, fsent->target, fsent->vfstype, fsent->data);
	
	if (write(mtab_fd, line, strlen(line)) == -1)
		goto error;
	
	free(line);
	close(mtab_fd);
	restore_root();
	return 0;
	
error:
	free(line);
	close(mtab_fd);
	restore_root();
	return -1;
}

static
int parse_fsent(struct mntspec *fsent, char *fstab_line)
{
	/* ignore leading whitespace characters */
	while (isspace(*fstab_line)) ++fstab_line;
	
	/* ignore comments and empty lines */
	if (*fstab_line == '#' || *fstab_line == '\0')
		return 1;
	
#define GET_STRING(PTR,ALLOW_EOL) \
	PTR = fstab_line; \
	while (!isspace(*fstab_line) && *fstab_line != '\0') ++fstab_line; \
	if (!ALLOW_EOL && *fstab_line == '\0') return -1; \
	*fstab_line++ = '\0'; \
	while (isspace(*fstab_line)) ++fstab_line;

	/* get column entries */
	GET_STRING(fsent->source, false)
	GET_STRING(fsent->target, false)
	GET_STRING(fsent->vfstype, false)
	GET_STRING(fsent->data, true)
	
#undef GET_STRING
	
	/* set safe mount flags */
	fsent->flags = MS_NODEV;
	
	/* perform sanity checks */
	if (strcmp(fsent->vfstype, "swap")   == 0) return 1;
	if (strcmp(fsent->vfstype, "devpts") == 0) fsent->flags |= MS_NODEV;
	if (strcmp(fsent->vfstype, "none")   == 0) fsent->vfstype = 0;
	
	/* check data */
	char *data = strdup(fsent->data);
	char *buf;

#define CHECK_DATA(DATA,FLAGS) \
	if (strcasecmp(buf, DATA) == 0) fsent->flags |= FLAGS;
	
	while ((buf = strsep(&data, ",")) != 0) {
		CHECK_DATA("bind",       MS_BIND)
		CHECK_DATA("rbind",      MS_BIND|MS_REC)
		CHECK_DATA("noatime",    MS_NOATIME)
		CHECK_DATA("nodev",      MS_NODEV)
		CHECK_DATA("noexec",     MS_NOEXEC)
		CHECK_DATA("nodiratime", MS_NODIRATIME)
		CHECK_DATA("nosuid",     MS_NOSUID)
		CHECK_DATA("ro",         MS_RDONLY)
		CHECK_DATA("sync",       MS_SYNCHRONOUS)
	}
	
#undef CHECK_DATA
	
	return 0;
}

static
int mount_fsent(struct mntspec *fsent, struct options *opts)
{
	char *cwd;
	cwd = getcwd(0, 0);
	
	if (secure_chdir(fsent->target) == -1)
		goto skip;
	
	if (mount(fsent->source, ".", fsent->vfstype, fsent->flags, fsent->data) == -1)
		goto error;
	
	chdir(cwd);
	free(cwd);
	
	if (!opts->nomtab && update_mtab(fsent, opts) == -1)
		perror("Failed to update mtab");
	
	return 0;
	
error:
	return -1;
	
skip:
	return -2;
}

static
int umount_fsent(struct mntspec *fsent)
{
	char *cwd;
	
	/* skip the root filesystem */
	if (strcmp(fsent->target, "/") == 0)
		goto skip;
	
	cwd = getcwd(0, 0);
	
	if (secure_chdir(fsent->target) == -1)
		goto skip;
	
	if (umount2(".", MNT_FORCE|MNT_DETACH) == -1) {
		if (errno == ENOENT)
			goto skip;
		else
			goto error;
	}
	
	chdir(cwd);
	free(cwd);
	
	return 0;
	
error:
	return -1;
	
skip:
	return -2;
}

int main(int argc, char *argv[])
{
	/* init program data */
	struct commands cmds = {
		.mount  = false,
		.umount = false,
	};
	
	struct options opts = {
		.fstab  = 0,
		.mtab   = 0,
		.rbind  = 0,
		.nomtab = false,
	};
	
	bool ok = true, passed = false;
	int c;
	
	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'M':
				cmds.mount = true;
				break;
			
			case 'U':
				cmds.umount = true;
				break;
			
			case 'f':
				opts.fstab = optarg;
				break;
			
			case 'm':
				opts.mtab = optarg;
				break;
			
			case 'r':
				opts.rbind = optarg;
				break;
			
			case 'n':
				opts.nomtab = true;
				break;
			
			DEFAULT_GETOPT
		}
	}
	
	/* init file descriptors */
	if ((cwd_fd  = open(".", O_RDONLY|O_DIRECTORY)) == -1)
		PEXIT("Failed to get file descriptor for cwd", EXIT_COMMAND);
	
	if ((root_fd = open("/", O_RDONLY|O_DIRECTORY)) == -1)
		PEXIT("Failed to get file descriptor for root", EXIT_COMMAND);
	
	if (cmds.mount) {
		if (opts.fstab == 0)
			goto rbind;
		
		int fstab_fd;
		off_t fstab_len;
		char *fstab_buf;
		char *fstab_line;
		
		/* open fstab */
		fstab_fd = open(opts.fstab, O_RDONLY);
		
		if (fstab_fd == -1)
			PEXIT("Failed to open fstab", EXIT_COMMAND);
		
		/* get fstab size */
		fstab_len = lseek(fstab_fd, 0, SEEK_END);
		
		if (fstab_len == -1)
			PEXIT("Failed to get fstab size", EXIT_COMMAND);
		
		if (lseek(fstab_fd, 0, SEEK_SET) == -1)
			PEXIT("Failed to set file descriptor for fstab", EXIT_COMMAND);
		
		/* save fstab to a buffer */
		fstab_buf = (char *) malloc(fstab_len+1);
		
		if (read(fstab_fd, fstab_buf, fstab_len) != fstab_len)
			PEXIT("Failed to read fstab", EXIT_COMMAND);
		
		fstab_buf[fstab_len+1] = '\0';
		close(fstab_fd);
		
		/* iterate through fstab and mount appropriate entries */
		while ((fstab_line = strsep(&fstab_buf, "\n")) != 0) {
			struct mntspec fsent;
			
			switch (parse_fsent(&fsent, fstab_line)) {
				case -1:
					printf("Failed to parse fstab line:\n%s\n", fstab_line);
					break;
				
				case 0:
					if (mount_fsent(&fsent, &opts) == -1)
						ok = false;
					else
						passed = true;
					break;
				
				default:
					break;
			}
		}
		
		free(fstab_buf);
		
rbind:
		if (opts.rbind == 0)
			goto out;
		
		/* remount <path> at / */
		struct mntspec rbindent = {
			.source  = opts.rbind,
			.target  = "/",
			.vfstype = 0,
			.flags   = MS_BIND|MS_REC,
			.data    = 0,
		};
		
		if (mount_fsent(&rbindent, &opts) == -1)
			ok = false;
		else
			passed = true;
		
		goto out;
	}
	
	if (cmds.umount) {
		if (opts.mtab == 0)
			goto out;
		
		int mtab_fd;
		off_t mtab_len;
		char *mtab_buf;
		char *mtab_line;
		
		/* open mtab */
		mtab_fd = open(opts.mtab, O_RDONLY);
		
		if (mtab_fd == -1)
			PEXIT("Failed to open mtab", EXIT_COMMAND);
		
		/* get mtab size */
		mtab_len = lseek(mtab_fd, 0, SEEK_END);
		
		if (mtab_len == -1)
			PEXIT("Failed to get mtab size", EXIT_COMMAND);
		
		if (lseek(mtab_fd, 0, SEEK_SET) == -1)
			PEXIT("Failed to set file descriptor for mtab", EXIT_COMMAND);
		
		/* save mtab to a buffer */
		mtab_buf = (char *) malloc(mtab_len+1);
		
		if (read(mtab_fd, mtab_buf, mtab_len) != mtab_len)
			PEXIT("Failed to read mtab", EXIT_COMMAND);
		
		mtab_buf[mtab_len+1] = '\0';
		close(mtab_fd);
		
		/* iterate through mtab and umount appropriate entries */
		while ((mtab_line = strsep(&mtab_buf, "\n")) != 0) {
			struct mntspec fsent;
			
			switch (parse_fsent(&fsent, mtab_line)) {
				case -1:
					printf("Failed to parse mtab line:\n%s\n", mtab_line);
					break;
				
				case 0:
					if (umount_fsent(&fsent) == -1)
						ok = false;
					else
						passed = true;
					break;
				
				default:
					break;
			}
		}
		
		free(mtab_buf);
		truncate(opts.mtab, 0);
		
		goto out;
	}
	
	cmd_help();
	goto out;
	
out:
	if (ok)
		exit(EXIT_SUCCESS);
	
	if (passed)
		exit(EXIT_COMMAND + 1);
	
	exit(EXIT_COMMAND + 2);
}
