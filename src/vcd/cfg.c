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

#include <stdlib.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <lucid/str.h>

#include "cfg.h"
#include "log.h"

void cfg_atexit(void)
{
	log_debug("[trace] %s", __FUNCTION__);
	
	cfg_free(cfg);
}

int cfg_validate_host(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result)
{
	log_debug("[trace] %s", __FUNCTION__);
	
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
	log_debug("[trace] %s", __FUNCTION__);
	
	long int port = strtol(value, NULL, 0);
	
	if (port < 1 || port > 65536) {
		cfg_error(cfg, "Port out of range for %s = '%s'", opt->name, value);
		return -1;
	}
	
	*(long int *) result = port;
	return 0;
}

int cfg_validate_timeout(cfg_t *cfg, cfg_opt_t *opt,
                         const char *value, void *result)
{
	log_debug("[trace] %s", __FUNCTION__);
	
	long int timeout = strtol(value, NULL, 0);
	
	if (timeout < 1 || timeout > 3600) {
		cfg_error(cfg, "Timeout out of range for %s = '%s'", opt->name, value);
		return -1;
	}
	
	*(long int *) result = timeout;
	return 0;
}

int cfg_validate_path(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result)
{
	log_debug("[trace] %s", __FUNCTION__);
	
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
