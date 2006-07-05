#ifndef _VSTATD_RRD
#define _VSTATD_RRD

#define ST_BUF 128

#define VS_LIM_VL 3
#define VS_LAVG_VL 3
#define VS_INFO_VL 1
#define VS_NET_VL 6

#include <vserver.h>

int vs_rrd_check (xid_t xid);
int vs_rrd_create (xid_t xid);

#endif
