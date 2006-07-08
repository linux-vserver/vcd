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

#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include "lucid.h"

#include "cfg.h"
#include "vstats.h"

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

int cfg_validate_graphs (cfg_t *cfg, cfg_opt_t *opt,
			const char *value, void *result)
{
	char *pch;
	int dur;

	pch = strchr(value, '-');
	dur = atoi(value);

	if (strcmp(pch, "-mins") == 0)
		*(long int *) result = 2;
	else if (strcmp(pch, "-hrs") == 0)
		*(long int *) result = 1;
	else if (strcmp(pch, "-days") == 0)
		*(long int *) result = 1;
	else if (strcmp(pch, "-months") == 0)
		*(long int *) result = 1;
	else if (strcmp(pch, "-years") == 0)
		*(long int *) result = 1;
	else {
		cfg_error(cfg, "Invalid value for option %s = '%s'", opt->name, value);
		return -1;
	}

	if (dur == 0) {
		cfg_error(cfg, "Invalid 0 duration for option %s = '%s'", opt->name, value);
		return -1;
	}
	else if ((dur < 30) && ( *(long int *) result == 2)) {
		cfg_error(cfg, "Minimal minutes resolution is 30, %s = '%s'", opt->name, value);
		return -1;
	}
	else if (vs_graphs_counter == VS_GRAPHS_VL) {
		cfg_error(cfg, "Max graphic definitions exceeded from %s = '%s'", opt->name, value);
		return -1;
	}

	GRAPHS[vs_graphs_counter].dur = dur;
	GRAPHS[vs_graphs_counter].res = pch;
	vs_graphs_counter++;

	return 0;
} 
