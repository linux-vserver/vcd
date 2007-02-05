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
#include <termios.h>
#include <sqlite3.h>

#include <lucid/log.h>
#include <lucid/printf.h>
#include <lucid/str.h>
#include <lucid/whirlpool.h>

static
void usage(int rc)
{
	printf("Usage: vxpasswd <vxdbfile> <username> [<password>]\n");
	exit(rc);
}

static
void read_password(char **password)
{
	struct termios tty, oldtty;

	/* save original terminal settings */
	if (tcgetattr(STDIN_FILENO, &oldtty) == -1)
		log_perror_and_die("tcgetattr");

	tty = oldtty;

	tty.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
	tty.c_cc[VMIN]  = 1;
	tty.c_cc[VTIME] = 0;

	/* apply new terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty) == -1)
		log_perror_and_die("tcsetattr");

	str_readline(STDIN_FILENO, password);

	/* apply old terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldtty) == -1)
		log_perror_and_die("tcsetattr");

	write(STDERR_FILENO, "\n", 1);
}

int main(int argc, char **argv)
{
	char *vxdbfile, *username, *whirlpool_password, *password = NULL;
	char *buf;
	sqlite3 *vxdb;

	if (argc < 3)
		usage(EXIT_FAILURE);

	/* logging */
	log_options_t log_options = {
		.log_ident = argv[0],
		.log_dest  = LOGD_STDERR,
		.log_opts  = LOGO_PRIO|LOGO_IDENT,
	};

	log_init(&log_options);

	vxdbfile = argv[1];
	username = argv[2];

	if (str_isempty(vxdbfile))
		log_error_and_die("invalid vxdbfile");

	if (sqlite3_open(vxdbfile, &vxdb) != SQLITE_OK)
		log_error_and_die("could not open database: %s", sqlite3_errmsg(vxdb));

	if (str_isempty(username) || !str_isalnum(username))
		log_error_and_die("invalid username: %s", username);

	if (argc >= 4)
		password = str_dup(argv[3]);

	if (!password) {
		write(STDERR_FILENO, "password: ", 10);
		read_password(&password);

		write(STDERR_FILENO, "repeat password: ", 17);
		read_password(&buf);

		if (!str_equal(password, buf)) {
			log_error("passwords don't match!");
			exit(EXIT_FAILURE);
		}
	}

	if (str_len(password) < 8)
		log_error_and_die("password too short: %s", username);

	whirlpool_password = whirlpool_digest(password);

	asprintf(&buf, "UPDATE user SET password = '%s' WHERE name = '%s'",
			whirlpool_password, username);

	if (sqlite3_exec(vxdb, buf, NULL, NULL, NULL) != SQLITE_OK)
		log_error_and_die("could not update the database: %s",
				sqlite3_errmsg(vxdb));

	if (sqlite3_changes(vxdb) < 1)
		log_error_and_die("no such user: %s", username);

	return EXIT_SUCCESS;
}
