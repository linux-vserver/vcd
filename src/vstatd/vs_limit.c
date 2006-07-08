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

#include <string.h>
#include <inttypes.h>
#include <rrd.h>

#include "vs_rrd.h"
#include "log.h"
#include "cfg.h"
#include "lucid.h"


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
		name = (char *) malloc(strlen(str));
		ret = sscanf(str, "%s %" SCNu64 " %" SCNu64 " %*c %" SCNu64, name, &cur, &min, &max);
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
	fclose(vfp);

	/* Resets min / max */
	if ((ret = vx_reset_rminmax(xid, NULL)) == -1) {
		log_error("cannot reset min / max, vserver xid '%d'", xid);
		return -2;
	}	
	return 0;
}


int vs_rrd_create_limit (xid_t xid, char *dbname, char *path) 
{
	char *db, start[ST_BUF];
	time_t curtime;
	
	curtime = time(NULL);
	snprintf(start, sizeof(start) - 1, "-b %d", (int) (curtime - curtime %30));

	db = path_concat(path, dbname);

	char *cargv[] = {
        	"create",
	        start,
	        db,
        	"-s 30",
	        "DS:lim_CUR:GAUGE:30:0:9223372036854775807",
        	"DS:lim_MIN:GAUGE:30:0:9223372036854775807", 
	        "DS:lim_MAX:GAUGE:30:0:9223372036854775807", 
        	"RRA:MIN:0:1:60",
	        "RRA:MAX:0:1:60", 
        	"RRA:AVERAGE:0:1:60",
	        "RRA:MIN:0:12:60",
        	"RRA:MAX:0:12:60", 
	        "RRA:AVERAGE:0:12:60", 
        	"RRA:MIN:0:48:60", 
	        "RRA:MAX:0:48:60",
        	"RRA:AVERAGE:0:48:60", 
	        "RRA:MIN:0:1440:60",
        	"RRA:MAX:0:1440:60", 
	        "RRA:AVERAGE:0:1440:60",
        	"RRA:MIN:0:17520:60",
	        "RRA:MAX:0:17520:60", 
        	"RRA:AVERAGE:0:17520:60",
	};
	int cargc = sizeof(cargv) / sizeof(*cargv), ret = 0;

	if ((ret = rrd_create(cargc, cargv)) < 0) {
		log_error("cannot create db '%s', vserver xid '%d': %s", db, xid, rrd_get_error());
		rrd_clear_error();
		return -1;
	} 
	return 0;
}  

int vs_rrd_update_limit (xid_t xid, char *dbname, char *name, char *path) 
{ 
	int i, ret, tm;
	char *db, *buf, *uargv[UARGC + 1];
	uint64_t cur = 0, min = 0, max = 0;
	time_t curtime;
	
	curtime = time(NULL);
	tm = (int) vs_rrd_gettime(curtime);

	for (i=0; LIMITS[i].name; i++) {
		if (strcmp(name, LIMITS[i].name) == 0) {
			cur = LIMITS[i].cur;
			min = LIMITS[i].min;
			max = LIMITS[i].max;
		}
	}

	db = path_concat(path, dbname);

	asprintf(&buf, "update %s %d:%" PRIu64 ":%" PRIu64 ":%" PRIu64, db, tm, cur, min, max);

	argv_from_str(buf, uargv, UARGC+1);

	if ((ret = rrd_update(UARGC, uargv)) == -1) {
		log_error("cannot update database '%s', vserver xid '%d': %s", db, xid, rrd_get_error());
		rrd_clear_error();
		return -1;
	}
	return 0;
}
