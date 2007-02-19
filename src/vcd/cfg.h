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

#ifndef _VCD_CFG_H
#define _VCD_CFG_H

#include <confuse.h>

extern cfg_t *cfg;

void cfg_load(const char *cfg_file);
void cfg_atexit(void);

int cfg_validate_host(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result);
int cfg_validate_port(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result);
int cfg_validate_timeout(cfg_t *cfg, cfg_opt_t *opt,
                         const char *value, void *result);
int cfg_validate_dir(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result);

#endif
