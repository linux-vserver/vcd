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

/*!
 * @file list.h
 * @brief List handling code from libvserver
 */
#ifndef _VLIST_H
#define _VLIST_H

#include <stdint.h>
#include <sys/types.h>

/*!
 * @defgroup list List functions
 * @{
 */
/*! 
 * @brief List node
 */
typedef struct list_node_s {
	char *key;  /*!< unique key identifier */
	void *data; /*!< any data associated with key */
} list_node_t;

/*!
 * @brief List
 */
typedef struct list_s {
	list_node_t *node; /*!< first node */
	size_t n;          /*!< number of nodes */
} list_t;

/*!
 * @brief Linked lists
 */
typedef struct list_link_s {
	list_t *p; /*!< prisitine list */
	list_t *d; /*!< descending list */
} list_link_t;

/*!
 * @file list.c
 * @brief List handling and parsing functions
 */
/*!
 * @defgroup list_alloc List allocation
 * @{
 */
/*!
 * @brief Allocate memory for a list
 * 
 * @param n Number of nodes
 * 
 * @return Pointer to empty list
 */
list_t *list_alloc(size_t n);

/*!
 * @brief Deallocate list memory
 * 
 * @param list List pointer
 */
void list_dealloc(list_t *list);

/*!
 * @brief Allocate memory for a list key
 * 
 * @param str Key to be allocated
 * 
 * @return Pointer to allocated memory
 * 
 * @see LIST_DATA_ALLOC_TYPE
 * @see list_set
 */
char *list_key_alloc(char *str);

/*!
 * @brief allocate memory for list data using TYPE
 * 
 * @see list_key_alloc
 * @see list_set
 */
#define LIST_DATA_ALLOC_TYPE(NAME, TYPE) \
static inline \
TYPE *NAME ## _list_data_alloc(TYPE value) \
{ \
	TYPE *data = (TYPE *)malloc(sizeof(TYPE)); \
	*data = value; \
	return data; \
}

/*!
 * @brief Set key/data pair
 * 
 * @param node Current list node
 * @param key  Key to be set
 * @param data Data to be set
 * 
 * @see list_key_alloc
 * @see LIST_DATA_ALLOC_TYPE
 */
void list_set(list_node_t *node, char *key, void *data);
/*! @} list_alloc */

/*!
 * @defgroup list_parse Token parser
 * @{
 */
/*!
 * @brief Count tokens
 * 
 * @param str   Token string
 * @param delim Token delimiter
 * 
 * @return Number of tokens
 */
size_t list_ntokens(const char *str, const char delim);

/*!
 * @brief Extract token and modify str to point to next token
 * 
 * @param str   Token string
 * @param delim Token delimiter 
 * 
 * @return Current token
 */
char *list_parse(const char **str, const char delim);

/*!
 * @brief Extract key/value pairs
 * 
 * @param str     Token string
 * @param delim   Token delimiter
 * @param kvdelim key/value delimiter
 * 
 * @return Token list pointer
 */
list_t *list_parse_hash(const char *str, const char delim, const char kvdelim);

/*!
 * @brief Extract tokens
 * 
 * @param str   Token string
 * @param delim Token delimiter
 * 
 * @return Token list pointer
 */
list_t *list_parse_list(const char *str, const char delim);
/*! @} list_parse */

/*!
 * @defgroup list_search List search
 * @{
 */
/*!
 * @brief Perform a linear search
 * 
 * @param list List pointer
 * @param key  Key to search for in \a list
 * 
 * @return Matching node pointer
 */
list_node_t *list_search(list_t *list, char *key);
/*! @} list_search */

/*!
 * @defgroup list_validate List validation
 * @{
 */
/*!
 * @brief Validate a token based flag list
 * 
 * @param link  Pointer to a linked flag list
 * @param clmod Clear flag modifier
 * 
 * @return 0 on success, -1 otherwise; errno will be set
 */
int list_validate_flag(list_link_t *link, const char clmod);

/*!
 * @brief Validate a token based list
 * 
 * @param link Pointer to a linked list
 * 
 * @return 0 on success, -1 otherwise; errno will be set
 */
int list_validate(list_link_t *link);
/*! @} list_validate */

/*!
 * @defgroup list_convert List converters
 * @{
 */
/*!
 * @brief Convert a token based flag list to flags and a set mask
 * 
 * @param link  Pointer to a linked list
 * @param clmod Clear flag modifier
 * @param flags Pointer to flags integer
 * @param mask  Pointer to mask integer
 */
void list_list2flags(list_link_t *link, const char clmod,
                     uint64_t *flags, uint64_t *mask);
/*! @} list_convert */

/*!
 * @defgroup list_defaults Default lists
 * @brief Default lists
 * 
 * @see list_alloc
 * @see list_key_alloc
 * @see LIST_DATA_ALLOC_TYPE
 * 
 * @{
 */
/*!
 * @file bcaps-list.c
 * @brief System capability list
 */
/*!
 * @brief Initialise system capability list
 * 
 * @see \htmlonly
 *   <a class="el" href="file:///usr/include/linux/capability.h">
 *     &lt;linux/capability.h&gt;
 *   </a>
 * \endhtmlonly
 * 
 * @return List of all system capabilities
 */
list_t *bcaps_list_init(void);

/*!
 * @file ccaps-list.c
 * @brief Context capability list
 */
/*!
 * @brief Initialise context capability list
 * 
 * @see \htmlonly
 *   <a class="el" href="file:///usr/include/linux/vserver/context.h">
 *     &lt;linux/vserver/context.h&gt;
 *   </a>
 * \endhtmlonly
 * 
 * @return List of all context capabilities
 */
list_t *ccaps_list_init(void);

/*!
 * @file cflags-list.c
 * @brief Context flag list
 */
/*!
 * @brief Initialise context flag list
 * 
 * @see \htmlonly
 *   <a class="el" href="file:///usr/include/linux/vserver/context.h">
 *     &lt;linux/vserver/context.h&gt;
 *   </a>
 * \endhtmlonly
 * 
 * @return List of all context flags
 */
list_t *cflags_list_init(void);

/*!
 * @file mflags-list.c
 * @brief Migration flag list
 */
/*!
 * @brief Initialise migration flag list
 *
 * @see \htmlonly
 *   <a class="el" href="file:///usr/include/linux/vserver/context.h">
 *     &lt;linux/vserver/context.h&gt;
 *   </a>
 * \endhtmlonly
 *
 * @return List of all migration flags
 */
list_t *mflags_list_init(void);

/*!
 * @file iattr-list.c
 * @brief Inode attribute list
 */
/*!
 * @brief Initialise inode attribute list
 * 
 * @see \htmlonly
 *   <a class="el" href="file:///usr/include/linux/vserver/inode.h">
 *     &lt;linux/vserver/inode.h&gt;
 *   </a>
 * \endhtmlonly
 * 
 * @return List of all inode attributes
 */
list_t *iattr_list_init(void);

/*!
 * @file nflags-list.c
 * @brief Network context flag list
 */
/*!
 * @brief Initialise network context flag list
 * 
 * @see \htmlonly
 *   <a class="el" href="file:///usr/include/linux/vserver/network.h">
 *     &lt;linux/vserver/network.h&gt;
 *   </a>
 * \endhtmlonly
 * 
 * @return List of all network context flags
 */
list_t *nflags_list_init(void);

/*!
 * @file rlimit-list.c
 * @brief Resource limit list
 */
/*!
 * @brief Initialise resource limit list
 * 
 * @see \htmlonly
 *   <a class="el" href="file:///usr/include/linux/vserver/limit.h">
 *     &lt;linux/vserver/limit.h&gt;
 *   </a>
 * \endhtmlonly
 * @see \htmlonly
 *   <a class="el" href="file:///usr/include/sys/resource.h">
 *     &lt;sys/resource.h&gt;
 *   </a>
 * \endhtmlonly
 * 
 * @return List of all resource limits
 */
list_t *rlimit_list_init(void);

/*!
 * @file sched-list.c
 * @brief Scheduler mask list
 */
/*!
 * @brief Initialise scheduler mask list
 * 
 * @see \htmlonly
 *   <a class="el" href="file:///usr/include/linux/vserver/sched_cmd.h">
 *     &lt;linux/vserver/sched_cmd.h&gt;
 *   </a>
 * \endhtmlonly
 * 
 * @return List of all scheduler masks
 */
list_t *sched_list_init(void);

/*!
 * @file vhi-list.c
 * @brief VHI name list
 */
/*!
 * @brief Initialise VHI name list
 * 
 * @see \htmlonly
 *   <a class="el" href="file:///usr/include/linux/vserver/cvirt_cmd.h">
 *     &lt;linux/vserver/cvirt_cmd.h&gt;
 *   </a>
 * \endhtmlonly
 * 
 * @return List of all VHI names
 */
list_t *vhi_list_init(void);
/*! @} list_defaults */

/*!
 * @defgroup list_iter List iteration
 * @brief Iterate through a list
 * 
 * @{
 */
/*!
 * @brief Iterate through LIST using IDX as counter
 */
#define list_foreach(LIST, IDX) for (size_t IDX = 0; IDX < (LIST)->n; IDX++)
/*! @} list_iter */

/*! @} list */

#endif /* _VLIST_H_ */
