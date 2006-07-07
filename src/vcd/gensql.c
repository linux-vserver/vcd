// Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>

#include "lucid.h"

static char VXDBSQL[] =
"CREATE TABLE IF NOT EXISTS dx_limit (\n"
"  xid SMALLINT NOT NULL,\n"
"  path TEXT NOT NULL,\n"
"  space INT NOT NULL,\n"
"  inodes INT NOT NULL,\n"
"  reserved TINYINT,\n"
"  UNIQUE(xid, path)\n"
");\n"
"CREATE TABLE IF NOT EXISTS init_method (\n"
"  xid SMALLINT NOT NULL,\n"
"  method TEXT NOT NULL,\n"
"  start TEXT,\n"
"  stop TEXT,\n"
"  timeout TINYINT,\n"
"  UNIQUE(xid)\n"
");\n"
"CREATE TABLE IF NOT EXISTS mount (\n"
"  xid SMALLINT NOT NULL,\n"
"  spec TEXT NOT NULL,\n"
"  path TEXT NOT NULL,\n"
"  vfstype TEXT,\n"
"  mntops TEXT,\n"
"  UNIQUE(xid, path)\n"
");\n"
"CREATE TABLE IF NOT EXISTS nx_addr (\n"
"  xid SMALLINT NOT NULL,\n"
"  addr TEXT NOT NULL,\n"
"  netmask TEXT NOT NULL,\n"
"  UNIQUE(xid, addr)\n"
");\n"
"CREATE TABLE IF NOT EXISTS nx_broadcast (\n"
"  xid SMALLINT NOT NULL,\n"
"  broadcast TEXT NOT NULL,\n"
"  UNIQUE(xid)\n"
");\n"
"CREATE TABLE IF NOT EXISTS user (\n"
"  uid SMALLINT NOT NULL,\n"
"  name TEXT NOT NULL,\n"
"  password TEXT NOT NULL,\n"
"  admin TINYINT NOT NULL,\n"
"  UNIQUE(uid),\n"
"  UNIQUE(name)\n"
");\n"
"CREATE TABLE IF NOT EXISTS user_caps (\n"
"  uid SMALLINT NOT NULL,\n"
"  cap TEXT NOT NULL,\n"
"  UNIQUE(uid, cap)\n"
");\n"
"CREATE TABLE IF NOT EXISTS vx_bcaps (\n"
"  xid SMALLINT NOT NULL,\n"
"  bcap TEXT NOT NULL,\n"
"  UNIQUE(xid, bcap)\n"
");\n"
"CREATE TABLE IF NOT EXISTS vx_ccaps (\n"
"  xid SMALLINT NOT NULL,\n"
"  ccap TEXT NOT NULL,\n"
"  UNIQUE(xid, ccap)\n"
");\n"
"CREATE TABLE IF NOT EXISTS vx_flags (\n"
"  xid SMALLINT NOT NULL,\n"
"  flag TEXT NOT NULL,\n"
"  UNIQUE(xid, flag)\n"
");\n"
"CREATE TABLE IF NOT EXISTS vx_limit (\n"
"  xid SMALLINT NOT NULL,\n"
"  type TEXT NOT NULL,\n"
"  soft BIGINT,\n"
"  max BIGINT,\n"
"  UNIQUE(xid, type)\n"
");\n"
"CREATE TABLE IF NOT EXISTS vx_sched (\n"
"  xid SMALLINT NOT NULL,\n"
"  cpuid SMALLINT NOT NULL,\n"
"  fillrate INT NOT NULL,\n"
"  fillrate2 INT NOT NULL,\n"
"  interval INT NOT NULL,\n"
"  interval2 INT NOT NULL,\n"
"  priobias INT NOT NULL,\n"
"  tokensmin INT NOT NULL,\n"
"  tokensmax INT NOT NULL,\n"
"  UNIQUE(xid, cpuid)\n"
");\n"
"CREATE TABLE IF NOT EXISTS vx_uname (\n"
"  xid SMALLINT NOT NULL,\n"
"  uname TEXT NOT NULL,\n"
"  value TEXT NOT NULL,\n"
"  UNIQUE(xid, uname)\n"
");\n"
"CREATE TABLE IF NOT EXISTS xid_name_map (\n"
"  xid SMALLINT NOT NULL,\n"
"  name TEXT NOT NULL,\n"
"  UNIQUE(xid),\n"
"  UNIQUE(name)\n"
");\n"
"CREATE TABLE IF NOT EXISTS xid_uid_map (\n"
"  xid SMALLINT NOT NULL,\n"
"  uid INT NOT NULL,\n"
"  UNIQUE(xid, uid)\n"
");\n";

static
void read_password(char **password)
{
	struct termios tty, oldtty;
	
	/* save original terminal settings */
	if (tcgetattr(STDIN_FILENO, &oldtty) == -1) {
		perror("tcgetattr");
		exit(EXIT_FAILURE);
	}
	
	tty = oldtty;
	
	tty.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
	tty.c_cc[VMIN]  = 1;
	tty.c_cc[VTIME] = 0;
	
	/* apply new terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty) == -1) {
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
	
	io_read_eol(STDIN_FILENO, password);
	
	/* apply old terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldtty) == -1) {
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
	
	write(STDERR_FILENO, "\n", 1);
}

int main(int argc, char **argv)
{
	char *username, *password, *sha1_pass;
	char *vshelper_password, *sha1_vshelper_pass;
	
	write(STDERR_FILENO, "admin username: ", 16);
	
	if (io_read_eol(STDIN_FILENO, &username) == -1) {
		perror("io_read_eol");
		exit(EXIT_FAILURE);
	}
	
	write(STDERR_FILENO, "admin password: ", 16);
	read_password(&password);
	
	write(STDERR_FILENO, "vshelper password: ", 19);
	read_password(&vshelper_password);
	
	sha1_pass = sha1_digest(password);
	sha1_vshelper_pass = sha1_digest(vshelper_password);
	
	printf("BEGIN TRANSACTION;\n");
	printf(VXDBSQL);
	printf("INSERT INTO user VALUES (1, '%s', '%s', 1);\n", username, sha1_pass);
	printf("INSERT INTO user VALUES (2, 'vshelper', '%s', 1);\n", sha1_vshelper_pass);
	printf("INSERT INTO user_caps VALUES(1, 'AUTH');\n");
	printf("INSERT INTO user_caps VALUES(1, 'DLIM');\n");
	printf("INSERT INTO user_caps VALUES(1, 'INIT');\n");
	printf("INSERT INTO user_caps VALUES(1, 'MOUNT');\n");
	printf("INSERT INTO user_caps VALUES(1, 'NET');\n");
	printf("INSERT INTO user_caps VALUES(1, 'BCAP');\n");
	printf("INSERT INTO user_caps VALUES(1, 'CCAP');\n");
	printf("INSERT INTO user_caps VALUES(1, 'CFLAG');\n");
	printf("INSERT INTO user_caps VALUES(1, 'RLIM');\n");
	printf("INSERT INTO user_caps VALUES(1, 'SCHED');\n");
	printf("INSERT INTO user_caps VALUES(1, 'UNAME');\n");
	printf("INSERT INTO user_caps VALUES(1, 'CREATE');\n");
	printf("INSERT INTO user_caps VALUES(1, 'HELPER');\n");
	printf("INSERT INTO user_caps VALUES(2, 'INIT');\n");
	printf("INSERT INTO user_caps VALUES(2, 'HELPER');\n");
	printf("COMMIT TRANSACTION;\n");
	
	exit(EXIT_SUCCESS);
}
