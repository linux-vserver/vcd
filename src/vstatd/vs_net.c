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
		name = (char *) malloc(strlen(str));		
		ret = sscanf(str, 
				"%s %" SCNu64 " %*c %" SCNu64 " %" SCNu64 " %*c %" SCNu64 " %" SCNu64 " %*c %" SCNu64, 
				name, &netr, &netr_b, &nets, &nets_b, &netf, &netf_b );
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

int vs_rrd_create_net (xid_t xid, char *dbname, char *path)
{
	char *db;
	db = path_concat(path, dbname);

	char *cargv[] = {
	        "create",
	        db,
        	"-s 30",
	        "DS:net_RECV:GAUGE:30:0:9223372036854775807",
	        "DS:net_RECV_B:GAUGE:30:0:9223372036854775807",
        	"DS:net_SEND:GAUGE:30:0:9223372036854775807",
	        "DS:net_SEND_B:GAUGE:30:0:9223372036854775807",
        	"DS:net_FAIL:GAUGE:30:0:9223372036854775807",
	        "DS:net_FAIL_B:GAUGE:30:0:9223372036854775807",
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

int vs_rrd_update_net (xid_t xid, char *dbname, char *name, char *path)
{
	int i, ret;
	char *db, *buf, *uargv[UARGC];
	uint64_t netr = 0, netr_b = 0, nets = 0, nets_b = 0, netf = 0, netf_b = 0;

	for (i=0; NET[i].name; i++) {
		if (strcmp(name, NET[i].name) == 0) {
			netr = NET[i].netr;
			netr_b = NET[i].netr_b;
			nets = NET[i].nets;
			nets_b = NET[i].nets_b;
			netf = NET[i].netf;
			netf_b = NET[i].netf_b;
		}
	}

	db = path_concat(path, dbname);

	asprintf(&buf, "update %s N:%" SCNu64 ":%" SCNu64 ":%" SCNu64 ":%" SCNu64 ":%" SCNu64 ":%" SCNu64, 
		db, netr, netr_b, nets, nets_b, netf, netf_b);

	argv_from_str(buf, uargv, UARGC+1);

	if ((ret = rrd_update(UARGC, uargv)) == -1) {
		log_error("cannot update database '%s', vserver xid '%d': %s", db, xid, rrd_get_error());
		rrd_clear_error();
		return -1;
	}
	return 0;
}
