#ifndef _VSTATD_VSTATS_H
#define _VSTATD_VSTATS_H

#include "collector.h"
#include <vserver.h>

#define LIMIT_FILE "limit"
#define INFO_FILE "cvirt"
#define NET_FILE "cacct"

int vstats_init (char *fp, xid_t xid, struct vs_limit MIN, struct vs_limit MAX, struct vs_limit AVG, struct vs_net NET, struct vs_info INFO);

#endif
