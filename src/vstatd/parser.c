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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <rrd.h>
#include <stdarg.h>
#include <inttypes.h>

#include "vstats.h"

#include "log.h"

int vs_parse_limit (xid_t xid) 
{
	FILE *vfp;
	char str[ST_BUF * 4], *name;	
	uint64_t cur, min, max;
	int i, ret;

	if ((vfp = fopen(LIMIT_FILE, "r")) == NULL) {
		log_error("cannot open '%s', vserver xid '%d'", LIMIT_FILE, xid);
		return -1;
	}
	
	while (fgets(str, sizeof(str) - 1, vfp)) {
		ret = sscanf(str, "%as %" SCNu64 " %" SCNu64 " %*c %" SCNu64, &name, &cur, &min, &max);
		for (i = 0; LIMITS[i].name; i++) {
			if (strcmp(name, LIMITS[i].name) == 0) {
				if (ret != VS_LIM_VL + 1) 
					return -1;
				LIMITS[i].cur = cur;
				LIMITS[i].min = min;
				LIMITS[i].max = max;
			}
		}
	}
	return 0;
}


int vs_parse_info (xid_t xid) 
{
	FILE *vfp;
	char str[ST_BUF * 4], *name;
	uint64_t value;
	int i, ret;

	if ((vfp = fopen(INFO_FILE, "r")) == NULL) {
		log_error("cannot open '%s', vserver xid '%d'", INFO_FILE, xid);
		return -1;
	}

	while (fgets(str, sizeof(str) - 1, vfp)) {
		ret = sscanf(str, "%as %" SCNu64, &name, &value);
		for (i = 0; INFO[i].name; i++) {
			if (strcmp(name, INFO[i].name) == 0) {
				if (ret != VS_INFO_VL + 1)
					return -1;
				INFO[i].value = value;
			}
		}
	}
	return 0;
}				


int vs_parse_loadavg (xid_t xid) 
{
	FILE *vfp;
	char str[ST_BUF * 4], *name;
	float omin, fmin, ftmin;
	int i, ret;

	if ((vfp = fopen(INFO_FILE, "r")) == NULL) {
	log_error("cannot open '%s', vserver xid '%d'", INFO_FILE, xid);
	return -1;
	}

	while (fgets(str, sizeof(str) - 1, vfp)) {
		ret = sscanf(str, "%as %f %f %f", &name, &omin, &fmin, &ftmin);
		for (i = 0; LAVG[i].name; i++) {
			if (strcmp(name, LAVG[i].name) == 0) {
				if (ret != VS_LAVG_VL + 1)
					return -1;
				LAVG[i].omin = omin;
				LAVG[i].fmin = fmin;
				LAVG[i].ftmin = ftmin;
			}
		}
	}
	return 0;
}


int vs_parse_net (xid_t xid)
{
	FILE *vfp;
	char str[ST_BUF * 4], *name;
	uint64_t netr, netr_b, nets, nets_b, netf, netf_b;
	int i, ret;

	if ((vfp = fopen(NET_FILE, "r")) == NULL) {
		log_error("cannot open '%s', vserver xid '%d'", NET_FILE, xid);
		return -1;
	}

	while (fgets(str, sizeof(str) - 1, vfp)) {
		ret = sscanf(str, 
				"%as %" SCNu64 " %*c %" SCNu64 " %" SCNu64 " %*c %" SCNu64 " %" SCNu64 " %*c %" SCNu64, 
				&name, &netr, &netr_b, &nets, &nets_b, &netf, &netf_b );
		for (i = 0; NET[i].name; i++) {
			if (strcmp(name, NET[i].name) == 0) {
				if (ret != VS_NET_VL + 1)
					return -1;
				NET[i].netr = netr;
				NET[i].netr_b = netr_b;
				NET[i].nets = nets;
				NET[i].nets_b = nets_b;
				NET[i].netf = netf;
				NET[i].netf_b = netf_b;
			}
		}
	}
	return 0;
}
