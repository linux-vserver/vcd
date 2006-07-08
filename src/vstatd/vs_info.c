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
		name = (char *) malloc(strlen(str));
		ret = sscanf(str, "%s %" SCNu64, name, &value);
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

int vs_rrd_create_info (xid_t xid, char *dbname, char *path) 
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
	        "DS:info_VALUE:GAUGE:30:0:9223372036854775807",
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

int vs_rrd_update_info (xid_t xid, char *dbname, char *name, char *path)
{
	int i, ret, tm;
	char *db, *buf, *uargv[UARGC + 1];
	uint64_t value = 0;
	time_t curtime;

	curtime = time(NULL);
	tm = (int) vs_rrd_gettime(curtime);


	for (i=0; INFO[i].name; i++) {
		if (strcmp(name, INFO[i].name) == 0)
			value = INFO[i].value;
	}

	db = path_concat(path, dbname);

	asprintf(&buf, "update %s %d:%" SCNu64, db, tm, value);

	argv_from_str(buf, uargv, UARGC+1);

	if ((ret = rrd_update(UARGC, uargv)) == -1) {
		log_error("cannot update database '%s', vserver xid '%d': %s", db, xid, rrd_get_error());
		rrd_clear_error();
		return -1;
	}
	return 0;
}
