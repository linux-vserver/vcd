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

#ifndef _VCD_CFG_H
#define _VCD_CFG_H

#include "confuse.h"

static cfg_opt_t _CFG_listen[] = {
	CFG_STR("type", "socket",            CFGF_NONE),
	CFG_STR("bind", "/var/run/vcd.sock", CFGF_NONE),
	CFG_END()
};

static cfg_opt_t _CFG_group[] = {
	CFG_STR_LIST("methods", NULL, CFGF_NONE),
	CFG_END()
};

static cfg_opt_t _CFG_user[] = {
	CFG_STR("group",  NULL, CFGF_NONE),
	CFG_STR("passwd", NULL, CFGF_NONE),
	CFG_END()
};

cfg_opt_t CFG[] = {
	CFG_SEC("listen",    _CFG_listen,    CFGF_NONE),
	CFG_SEC("group",     _CFG_group,     CFGF_MULTI|CFGF_TITLE),
	CFG_SEC("user",      _CFG_user,      CFGF_MULTI|CFGF_TITLE),
	CFG_END()
};

#endif
