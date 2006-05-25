/***************************************************************************
 *   Copyright 2005 The libvserver Team                                    *
 *   See AUTHORS for details                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vserver.h>
#include "vlist.h"

#include <stdlib.h>

LIST_DATA_ALLOC_TYPE(cflags, uint64_t)

/*!
 * @brief Macro for cflags list allocation
 * @ingroup list_defaults
 */
#define LIST_ADD_CFLAG(TYPE, VALUE) \
list_set(p->node+(i++), \
         list_key_alloc(#VALUE), \
         cflags_list_data_alloc(TYPE ## _ ## VALUE));
	
list_t *cflags_list_init(void)
{
	list_t *p = list_alloc(25);
	
	int i = 0;
	LIST_ADD_CFLAG(VXF, INFO_LOCK)
	LIST_ADD_CFLAG(VXF, INFO_SCHED)
	LIST_ADD_CFLAG(VXF, INFO_NPROC)
	LIST_ADD_CFLAG(VXF, INFO_PRIVATE)
	
	LIST_ADD_CFLAG(VXF, INFO_INIT)
	LIST_ADD_CFLAG(VXF, INFO_HIDE)
	LIST_ADD_CFLAG(VXF, INFO_ULIMIT)
	LIST_ADD_CFLAG(VXF, INFO_NSPACE)
	
	LIST_ADD_CFLAG(VXF, SCHED_HARD)
	LIST_ADD_CFLAG(VXF, SCHED_PRIO)
	LIST_ADD_CFLAG(VXF, SCHED_PAUSE)
	
	LIST_ADD_CFLAG(VXF, VIRT_MEM)
	LIST_ADD_CFLAG(VXF, VIRT_UPTIME)
	LIST_ADD_CFLAG(VXF, VIRT_CPU)
	LIST_ADD_CFLAG(VXF, VIRT_LOAD)
	
	LIST_ADD_CFLAG(VXF, HIDE_MOUNT)
	LIST_ADD_CFLAG(VXF, HIDE_NETIF)
	
	LIST_ADD_CFLAG(VXF, STATE_SETUP)
	LIST_ADD_CFLAG(VXF, STATE_INIT)
	
	LIST_ADD_CFLAG(VXF, SC_HELPER)
	LIST_ADD_CFLAG(VXF, REBOOT_KILL)
	LIST_ADD_CFLAG(VXF, PERSISTENT)
	
	LIST_ADD_CFLAG(VXF, FORK_RSS)
	LIST_ADD_CFLAG(VXF, PROLIFIC)
	
	LIST_ADD_CFLAG(VXF, IGNEG_NICE)
	
/*	LIST_ADD_CFLAG(VXF, ONE_TIME) */
	
/*	LIST_ADD_CFLAG(VXF, INIT_SET) */
	
	return p;
}
