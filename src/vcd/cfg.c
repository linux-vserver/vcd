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

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "lucid.h"

#include "cfg.h"

int cfg_validate_host(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result)
{
	struct in_addr inaddr;
	
	if (inet_pton(AF_INET, value, &inaddr) == 0) {
		cfg_error(cfg, "Invalid address for %s = '%s'", opt->name, value);
		return -1;
	}
	
	*(const char **) result = (const char *) value;
	return 0;
}

int cfg_validate_port(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result)
{
	long int port = strtol(value, NULL, 0);
	
	if (port < 1 || port > 65536) {
		cfg_error(cfg, "Port out of range for %s = '%s'", opt->name, value);
		return -1;
	}
	
	*(long int *) result = port;
	return 0;
}

int cfg_validate_tls(cfg_t *cfg, cfg_opt_t *opt,
                     const char *value, void *result)
{
	if (strcmp(value, "none") == 0)
		*(long int *) result = 0;
	else if (strcmp(value, "anonymous") == 0)
		*(long int *) result = 1;
	else if (strcmp(value, "x509") == 0)
		*(long int *) result = 2;
	else {
		cfg_error(cfg, "Invalid TLS mode '%s'", value);
		return -1;
	}
	
	return 0;
}

int cfg_validate_log(cfg_t *cfg, cfg_opt_t *opt,
                     const char *value, void *result)
{
	if (strcmp(value, "error") == 0)
		*(long int *) result = 1;
	else if (strcmp(value, "warn") == 0)
		*(long int *) result = 2;
	else if (strcmp(value, "info") == 0)
		*(long int *) result = 3;
	else if (strcmp(value, "debug") == 0)
		*(long int *) result = 4;
	else {
		cfg_error(cfg, "Invalid log level '%s'", value);
		return -1;
	}
	
	return 0;
}

int cfg_validate_path(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result)
{
	struct stat sb;
	
	if (!str_path_isabs(value)) {
		cfg_error(cfg, "Invalid absolute path for %s = '%s'", opt->name, value);
		return -1;
	}
	
	if (lstat(value, &sb) == -1) {
		cfg_error(cfg, "File does not exist for %s = '%s'", opt->name, value);
		return -1;
	}
	
	*(const char **) result = (const char *) value;
	return 0;
}
