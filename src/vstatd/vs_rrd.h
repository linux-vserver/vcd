#ifndef _VSTATD_RRD
#define _VSTATD_RRD

#include <vserver.h>
#include "vstats.h"

int vs_rrd_check (xid_t xid);
int vs_rrd_create (xid_t xid);
int vs_rrd_update (xid_t xid, struct vs_limit CUR, struct vs_limit MIN, struct vs_limit MAX, struct vs_info INFO, struct vs_net NET);

#endif
