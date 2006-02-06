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

#include <unistd.h>
#include <fcntl.h>

#include "vc.h"

/* go to <dir> in <cwd> as root, while <root> is the original root.
** going into the chroot before doing chdir(dir) prevents symlink attacks
** and hence is safer */
int vc_secure_chdir(int rootfd, int cwdfd, char *dir)
{
	int dirfd;
	
	/* check cwdfd */
	if (fchdir(cwdfd) == -1)
		return -1;
	
	/* chroot to cwd */
	if (chroot(".") == -1)
		return -1;
	
	/* now go to dir in the chroot */
	if (chdir(dir) == -1)
		return -1;
	
	/* save a file descriptor of the target dir */
	dirfd = open(".", O_RDONLY|O_DIRECTORY);
	
	if (dirfd == -1)
		return -1;
	
	/* break out of the chroot */
	if (fchdir(rootfd) == -1)
		goto error;
	
	if (chroot(".") == -1)
		goto error;
	
	/* now go to the saved target dir (but outside the chroot) */
	if (fchdir(dirfd) == -1)
		goto error;
	
	close(dirfd);
	return 0;
	
error:
	close(dirfd);
	return -1;
}
