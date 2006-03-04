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

LIST_DATA_ALLOC_TYPE(sched, uint64_t)

/*!
 * @brief Macro for sched list allocation
 * @ingroup list_defaults
 */
#define LIST_ADD_SCHED(TYPE, VALUE) \
list_set(p->node+(i++), \
         list_key_alloc(#VALUE), \
         sched_list_data_alloc(TYPE ## _ ## VALUE));
	
list_t *sched_list_init(void)
{
	list_t *p = list_alloc(6);
	
	int i = 0;
	LIST_ADD_SCHED(VXSM, FILL_RATE)
	LIST_ADD_SCHED(VXSM, INTERVAL)
	LIST_ADD_SCHED(VXSM, TOKENS)
	LIST_ADD_SCHED(VXSM, TOKENS_MIN)
	LIST_ADD_SCHED(VXSM, TOKENS_MAX)
	LIST_ADD_SCHED(VXSM, PRIO_BIAS)
	
	return p;
}
