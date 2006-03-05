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
#include <mntent.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <limits.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <vserver.h>
#include <sys/wait.h>
#include <unistd.h>

#include "printf.h"
#include "tools.h"

#define NAME  "vmount"
#define DESCR "Filesystem Manager"

#define SHORT_OPTS "LMURf:m:r:n"

/* sys/mount.h does not define MS_REC! */
#ifndef MS_REC
#define MS_REC 16384
#endif

/* sys/mount.h does not set MNT_DETACH! */
#ifndef MNT_DETACH
#define MNT_DETACH 0x00000002
#endif

typedef enum {
	VMNT_LIST = 0,   	/* List mountpoints */
	VMNT_MOUNT = 1,  	/* Mount fstab entries */
	VMNT_UMOUNT = 2, 	/* Unmount fstab entries */
	VMNT_RMOUNT = 3 	/* Rbind/Move mount guest's root to / */
} command_t;

struct options {
	GLOBAL_OPTS;
	command_t cmd;
	char *fstab;
	char *mtab;
	char *groot;
	bool nomtab;
};

struct mntspec {
	char *source;
	char *target;
	char *vfstype;
	char *options;
	int pass;
	struct mntspec *next;
};

int cwd_fd;
int root_fd;

static inline
void cmd_help()
{
	vu_printf("Usage: %s <command> <opts>*\n"
	            "\n"
	            "Available commands:\n"
	            "    -L            List mountpoints\n"
	            "    -M            Mount vserver filesystems\n"
	            "    -U            Unmount vserver filesystems\n"
	            "    -R            Remount guest's root to /\n"
	            "\n"
	            "Available options:\n"
		    GLOBAL_HELP
	            "    -f <file>     Use fstab <file>\n"
	            "    -m <file>     Use mtab <file>\n"
	            "    -r <path>     Path to guest's root (defaults to '.')\n"
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
	
	vu_asprintf(&line, "%s %s %s %s 0 0\n", fsent->source, fsent->target,
		fsent->vfstype ? fsent->vfstype : "none",
		fsent->options ? fsent->options : "defaults");
	
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
struct mntspec *load_fstab(const char *file)
{
	FILE *f;

	if ((f = setmntent(file ? file : "./etc/fstab", "r")))
	{
		struct mntent *line;
		struct mntspec *last = NULL;
		struct mntspec *first = NULL;
		int n = 0;
		while ((line = getmntent(f))) {
			struct mntspec *tmp = malloc(sizeof(struct mntspec));
			if (tmp == NULL)
				continue;
			tmp->target = strdup(line->mnt_dir);
			tmp->source = strdup(line->mnt_fsname);
			tmp->options = strdup(line->mnt_opts);
			tmp->vfstype = strdup(line->mnt_type);
			tmp->pass = line->mnt_passno;
			tmp->next = NULL;
			if (first && last)
				last->next = tmp;
			else
				first = tmp;
			last = tmp;
			n++;
		}
		endmntent(f);
		return first;
	}
	return NULL;
}

static
int mount_root(struct options *opts)
{
	char cwd[PATH_MAX];
	
	if (getcwd(cwd, PATH_MAX) == NULL)
		goto error;
	
	if (secure_chdir(opts->groot) == -1)
		goto skip;
	
	VPRINTF(opts, "Remounting guest's root '%s' to /\n", opts->groot);

	if (mount(opts->groot, "/", NULL, MS_BIND|MS_REC, NULL) == -1)
		goto error;
	
	chdir(cwd);
	
	return 0;
	
error:
	return -1;
	
skip:
	return -2;
}

static
int mount_fs(struct mntspec *fsent, struct options *opts)
{
	char cwd[PATH_MAX];
	
	if (getcwd(cwd, PATH_MAX) == NULL)
		goto error;
	
	if (secure_chdir(fsent->target) == -1)
		goto skip;
	
	/* fork ps */
	int c;
	pid_t pid = fork();

	if (pid == 0) {
		char mcwd[PATH_MAX];
		if (getcwd(mcwd, PATH_MAX) == NULL)
			goto error;
		VPRINTF(opts, "Mounting '%s' on '%s', type '%s' from '%s'\n", fsent->source, fsent->target, fsent->vfstype, mcwd);
		// mount source target -t fs -o opts
		if (fsent->options && fsent->options[0]) {
			if (execl("/bin/mount", "mount", fsent->source, ".", "-t", fsent->vfstype, "-n", "-o", fsent->options, (char*)NULL) == -1)
				PEXIT("Failed to start mount", EXIT_COMMAND);
		} else {
			// If there are no options, don't provide empty argument to mount
			if (execl("/bin/mount", "mount", fsent->source, ".", "-t", fsent->vfstype, "-n", (char*)NULL) == -1)
				PEXIT("Failed to start mount", EXIT_COMMAND);
		}
	} else if (pid == -1)
		PEXIT("Failed to clone process", EXIT_COMMAND);

	waitpid(pid, &c, 0);
	if ((WIFEXITED(c) ? WEXITSTATUS(c) : 1) != 0)
		goto error;

	chdir(cwd);
	
	if (!opts->nomtab && update_mtab(fsent, opts) == -1)
		perror("Failed to update mtab");
	
	return 0;
	
error:
	vu_printf("Mount failed for '%s' type '%s': %s\n", fsent->target, fsent->vfstype, strerror(errno));
	return -1;
	
skip:
	return -2;
}

static
int umount_fs(struct mntspec *fsent, struct options *opts)
{
	char cwd[PATH_MAX];
	
	/* skip the root filesystem */
	if (strcmp(fsent->target, "/") == 0)
		goto skip;
	
	if (getcwd(cwd, PATH_MAX) == NULL)
		goto error;
	
	if (secure_chdir(fsent->target) == -1)
		goto skip;
	
	VPRINTF(opts, "Unmounting '%s' \n", fsent->target);
	if (umount2(".", MNT_FORCE|MNT_DETACH) == -1) {
		if (errno == ENOENT)
			goto skip;
		else
			goto error;
	}
	
	chdir(cwd);
	
	return 0;
	
error:
	return -1;
	
skip:
	return -2;
}

int main(int argc, char *argv[])
{
	struct options opts = {
		GLOBAL_OPTS_INIT,
		.cmd    = VMNT_LIST,
		.fstab  = NULL,
		.mtab   = NULL,
		.groot  = NULL,
		.nomtab = false,
	};
	
	bool ok = true, passed = false;
	int c;
	
	DEBUGF("%s: starting ...\n", NAME);

	/* parse command line */
	while (1) {
		c = getopt(argc, argv, GLOBAL_CMDS SHORT_OPTS);
		if (c == -1) break;
		
		switch (c) {
			GLOBAL_CMDS_GETOPT
			
			case 'M':
				opts.cmd = VMNT_MOUNT;
				break;
			
			case 'U':
				opts.cmd = VMNT_UMOUNT;
				break;
			
			case 'L':
				opts.cmd = VMNT_LIST;
				break;
			
			case 'R':
				opts.cmd = VMNT_RMOUNT;
				break;
			
			case 'f':
				opts.fstab = optarg;
				break;
			
			case 'm':
				opts.mtab = optarg;
				break;
			
			case 'r':
				opts.groot = optarg;
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
	
	switch (opts.cmd) {
		case VMNT_MOUNT: {
			/* Mount fstab content */
			struct mntspec *fstab = load_fstab(opts.fstab);
			while (fstab) {
				if (mount_fs(fstab, &opts) == -1)
					ok = false;
				else
					passed = true;
				struct mntspec *tmp = fstab;
				fstab = fstab->next;
				free(tmp->source);
				free(tmp->target);
				free(tmp->vfstype);
				free(tmp->options);
				free(tmp);
			}
			break;
		}
		case VMNT_UMOUNT: {
			/* Unmount fstab content */
			struct mntspec *mtab = load_fstab(opts.mtab);
			while (mtab) {
				if (umount_fs(mtab, &opts) == -1)
					ok = false;
				else
					passed = true;
				struct mntspec *tmp = mtab;
				mtab = mtab->next;
				free(tmp->source);
				free(tmp->target);
				free(tmp->vfstype);
				free(tmp->options);
				free(tmp);
			}
			
			if (opts.mtab)
				truncate(opts.mtab, 0);
			break;
		}
		case VMNT_RMOUNT: {
			/* Remount guest's root to / */
			if (mount_root(&opts) == -1)
				ok = false;
			else
				passed = true;
			break;
		}
		default: {
			/* List mounted filesystems */
			struct mntspec *mtab = load_fstab(opts.mtab);
			while (mtab) {
				vu_printf("%s\t%s\t%s\t%s\n", mtab->source, mtab->target, mtab->vfstype, mtab->options);
				struct mntspec *tmp = mtab;
				mtab = mtab->next;
				free(tmp->source);
				free(tmp->target);
				free(tmp->vfstype);
				free(tmp->options);
				free(tmp);
			}
			break;
		}
	}
	
	goto out;
	cmd_help();
	
out:
	if (ok)
		exit(EXIT_SUCCESS);
	
	if (passed)
		exit(EXIT_COMMAND + 1);
	
	exit(EXIT_COMMAND + 2);
}
