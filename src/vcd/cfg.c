// Copyright 2006-2007 Benedikt Böhm <hollow@gentoo.org>
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
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "cfg.h"

#include <lucid/addr.h>
#include <lucid/log.h>
#include <lucid/misc.h>
#include <lucid/printf.h>
#include <lucid/scanf.h>
#include <lucid/str.h>

static cfg_opt_t CFG_OPTS[] = {
	CFG_STR_CB("host",    "127.0.0.1", CFGF_NONE, &cfg_validate_host),
	CFG_INT_CB("port",    13386,       CFGF_NONE, &cfg_validate_port),
	CFG_INT_CB("timeout", 30,          CFGF_NONE, &cfg_validate_timeout),

	CFG_STR("logfile", NULL, CFGF_NONE),
	CFG_STR("pidfile", NULL, CFGF_NONE),

	CFG_STR_CB("datadir",     LOCALSTATEDIR "/vcd",   CFGF_NONE,
			&cfg_validate_dir),
	CFG_STR_CB("templatedir", VBASEDIR "/templates/", CFGF_NONE,
			&cfg_validate_dir),
	CFG_STR_CB("vbasedir",    VBASEDIR,               CFGF_NONE,
			&cfg_validate_dir),

	CFG_END()
};

cfg_t *cfg;

void cfg_load(const char *cfg_file)
{
	cfg = cfg_init(CFG_OPTS, CFGF_NOCASE);

	switch (cfg_parse(cfg, cfg_file)) {
	case CFG_FILE_ERROR:
		log_perror("cfg_parse(%s)", cfg_file);
		dprintf(STDERR_FILENO, "cfg_parse(%s): %s\n",
				cfg_file, strerror(errno));
		exit(EXIT_FAILURE);

	case CFG_PARSE_ERROR:
		log_error("cfg_parse(%s): parse error", cfg_file);
		dprintf(STDERR_FILENO, "cfg_parse(%s): parse error\n", cfg_file);
		exit(EXIT_FAILURE);

	default:
		break;
	}
}

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
