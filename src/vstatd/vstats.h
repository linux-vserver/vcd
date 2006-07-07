// Copyright 2006 Remo Lemma <coloss7@gmail.com>
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

#ifndef _VSTATD_VSTATS_H
#define _VSTATD_VSTATS_H

#include <vserver.h>

#define LIMIT_FILE "limit"
#define INFO_FILE "cvirt"
#define NET_FILE "cacct"

#define ST_BUF 512

#define VS_LIM_VL 3
#define VS_LAVG_VL 3
#define VS_INFO_VL 1
#define VS_NET_VL 6
#define VS_ALL 70

typedef struct {
	char *name;
	uint64_t cur;
	uint64_t min;
	uint64_t max;
} vstats_limit_t;

typedef struct {
	char *name;
	uint64_t value;
} vstats_info_t;

typedef struct {
	char *name;
	float omin;
	float fmin;
	float ftmin;
} vstats_loadavg_t;

typedef struct {
	char *name;
	uint64_t netr;
	uint64_t netr_b;
	uint64_t nets;
	uint64_t nets_b;
	uint64_t netf;
	uint64_t netf_b;
} vstats_net_t;

extern vstats_limit_t LIMITS[];
extern vstats_info_t INFO[];
extern vstats_loadavg_t LAVG[];
extern vstats_net_t NET[];


int vs_parse_limit (xid_t xid);
int vs_parse_info (xid_t xid);
int vs_parse_loadavg (xid_t xid);
int vs_parse_net (xid_t xid);

#endif
