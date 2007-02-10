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

#include <errno.h>
#include <sys/stat.h>

#include "cfg.h"

#include <lucid/addr.h>
#include <lucid/log.h>
#include <lucid/misc.h>
#include <lucid/scanf.h>
#include <lucid/str.h>

void cfg_atexit(void)
{
	LOG_TRACEME
	cfg_free(cfg);
}

int cfg_validate_host(cfg_t *cfg, cfg_opt_t *opt,
		const char *value, void *result)
{
	LOG_TRACEME

	if (addr_from_str(value, NULL, NULL) < 1) {
		cfg_error(cfg, "%s(%s) is not a valid address", opt->name, value);
		return -1;
	}

	*(const char **) result = (const char *) value;
	return 0;
}

int cfg_validate_port(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result)
{
	LOG_TRACEME

	int port;

	if (sscanf(value, "%d", &port) < 1) {
		cfg_error(cfg, "%s(%s) is not a number", opt->name, value);
		return -1;
	}

	if (port < 1 || port > 65536) {
		cfg_error(cfg, "%s(%d) out of range (1-65536)", opt->name, port);
		return -1;
	}

	*(long int *) result = port;
	return 0;
}

int cfg_validate_timeout(cfg_t *cfg, cfg_opt_t *opt,
                         const char *value, void *result)
{
	LOG_TRACEME

	int timeout;

	if (sscanf(value, "%d", &timeout) < 1) {
		cfg_error(cfg, "%s(%s) is not a number", opt->name, value);
		return -1;
	}

	if (timeout < 1 || timeout > 3600) {
		cfg_error(cfg, "%s(%d) out of range (1-3600)", opt->name, timeout);
		return -1;
	}

	*(long int *) result = timeout;
	return 0;
}

int cfg_validate_dir(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result)
{
	LOG_TRACEME

	struct stat sb;

	if (!str_path_isabs(value)) {
		cfg_error(cfg, "%s is not a valid, absolute path", opt->name);
		return -1;
	}

	if (lstat(value, &sb) == -1) {
		if (errno == ENOENT) {
			log_warn("%s(%s) does not exist, creating it",
					opt->name, value);
			
			if (mkdirp(value, 0755) == -1)
				log_perror_and_die("mkdirp(%s)", value);
		}

		else
			log_perror_and_die("lstat(%s)", value);
	}

	else if (!S_ISDIR(sb.st_mode))
		log_error_and_die("%s(%s) exists, but is not a directory",
				opt->name, value);

	*(const char **) result = (const char *) value;
	return 0;
}
