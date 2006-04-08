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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "tools.h"

static const char *rcsid = "$Id$";

static
struct option long_opts[] = {
	COMMON_LONG_OPTS
	{ "create",  1, 0, 0x10 },
	{ "list",    1, 0, 0x11 },
	{ "fetch",   1, 0, 0x12 },
	{ "insert",  1, 0, 0x13 },
	{ "replace", 1, 0, 0x14 },
	{ "delete",  1, 0, 0x15 },
	{ NULL,      0, 0, 0 },
};

static inline
void usage(int rc)
{
	printf("Usage:\n\n"
	          "vdbm -create  <basename>\n"
	          "     -list    <basename>\n"
	          "     -fetch   <basename> <key>\n"
	          "     -insert  <basename> <key> <value>\n"
	          "     -replace <basename> <key> <value>\n"
	          "     -delete  <basename> <key>\n");
	exit(rc);
}

int main(int argc, char *argv[])
{
	INIT_ARGV0
	
	int c;
	char *basename;
	SDBM *db;
	DATUM key, val;
	
#define CASE_GOTO(ID, P) case ID: basename = optarg; goto P; break
	
	/* parse command line */
	while (GETOPT(c)) {
		switch (c) {
			COMMON_GETOPT_CASES
			
			CASE_GOTO(0x10, create);
			CASE_GOTO(0x11, list);
			CASE_GOTO(0x12, fetch);
			CASE_GOTO(0x13, insert);
			CASE_GOTO(0x14, replace);
			CASE_GOTO(0x15, delete);
			
			DEFAULT_GETOPT_CASES
		}
	}
	
#undef CASE_GOTO
	
	goto usage;
	
create:
	if ((db = sdbm_open(basename, O_RDWR|O_CREAT, 0600)) == NULL)
		perr("sdbm_open");
	
	goto out;

list:
	if ((db = sdbm_open(basename, O_RDONLY, 0)) == NULL)
		perr("sdbm_open");
	
	for (key = sdbm_firstkey(db); key.dsize > 0; key = sdbm_nextkey(db)) {
		val = sdbm_fetch(db, key);
		write(STDOUT_FILENO, key.dptr, key.dsize);
		write(STDOUT_FILENO, " = '", 4);
		write(STDOUT_FILENO, val.dptr, val.dsize);
		write(STDOUT_FILENO, "'\n", 2);
	}
	
	goto out;
	
fetch:
	if (argc <= optind)
		goto usage;
	
	if ((db = sdbm_open(basename, O_RDONLY, 0)) == NULL)
		perr("sdbm_open");
	
	key.dptr  = argv[optind];
	key.dsize = strlen(key.dptr);
	
	val = sdbm_fetch(db, key);
	
	if (val.dsize > 0) {
		write(STDOUT_FILENO, val.dptr, val.dsize);
		write(STDOUT_FILENO, "\n", 1);
	}
	
	goto out;
	
insert:
	if (argc <= optind + 1)
		goto usage;
	
	if ((db = sdbm_open(basename, O_RDWR, 0)) == NULL)
		perr("sdbm_open");
	
	key.dptr  = argv[optind];
	key.dsize = strlen(key.dptr);
	
	val.dptr  = argv[optind+1];
	val.dsize = strlen(val.dptr);
	
	if (sdbm_store(db, key, val, SDBM_INSERT) == -1) {
		pwarn("sdbm_store");
		sdbm_close(db);
		exit(EXIT_FAILURE);
	}
	
	goto out;
	
replace:
	if (argc <= optind + 1)
		goto usage;
	
	if ((db = sdbm_open(basename, O_RDWR, 0)) == NULL)
		perr("sdbm_open");
	
	key.dptr  = argv[optind];
	key.dsize = strlen(key.dptr);
	
	val.dptr  = argv[optind+1];
	val.dsize = strlen(val.dptr);
	
	if (sdbm_store(db, key, val, SDBM_REPLACE) == -1) {
		pwarn("sdbm_store");
		sdbm_close(db);
		exit(EXIT_FAILURE);
	}
	
	goto out;
	
delete:
	if (argc <= optind)
		goto usage;
	
	if ((db = sdbm_open(basename, O_RDWR, 0)) == NULL)
		perr("sdbm_open");
	
	key.dptr  = argv[optind];
	key.dsize = strlen(key.dptr);
	
	if (sdbm_delete(db, key) == -1) {
		pwarn("sdbm_delete");
		sdbm_close(db);
		exit(EXIT_FAILURE);
	}
	
	goto out;
	
usage:
	usage(EXIT_FAILURE);

out:
	sdbm_close(db);
	exit(EXIT_SUCCESS);
}
