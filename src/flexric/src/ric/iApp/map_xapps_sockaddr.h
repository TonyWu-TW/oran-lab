/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef MAP_XAPPS_SOCKADDR_H
#define MAP_XAPPS_SOCKADDR_H 

#include "../../lib/e2ap/e2ap_global_node_id_wrapper.h"
#include "../../util/alg_ds/ds/assoc_container/assoc_generic.h"
#include "../../util/alg_ds/ds/assoc_container/bimap.h"
#include "../../lib/ep/sctp_msg.h"

#include <netinet/in.h>
#include <pthread.h>

typedef struct{
  // assoc_rb_tree_t tree; // key: uint16_t xapp_id | value: sctp_info_t  
  // It is read when dispatching by E2AP RIC INDICATION msgs, or SCTP_SHUTDOWN 
  // while written when new subscriptions arrived 

  bi_map_t bimap; // left:  key1: uint16_t xapp_id | value: sctp_info_t 
                  // right: key2: sctp_info_t      | value:  uint16_t xapp_id

  pthread_rwlock_t rw;
} map_xapps_sockaddr_t ; 

void init_map_xapps_sad(map_xapps_sockaddr_t* m);

void free_map_xapps_sad(map_xapps_sockaddr_t* m);

void add_map_xapps_sad(map_xapps_sockaddr_t* m, uint16_t xapp_id, sctp_info_t* s);

void rm_map_xapps_sad(map_xapps_sockaddr_t* m, uint16_t xapp_id);

sctp_info_t find_map_xapps_sad(map_xapps_sockaddr_t* m, uint16_t xapp_id);

uint16_t find_map_xapps_xid(map_xapps_sockaddr_t* m, sctp_info_t const* s);

#endif



