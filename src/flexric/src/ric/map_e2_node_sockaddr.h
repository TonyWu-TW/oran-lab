
/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef MAP_E2_NODE_SOCKADDR_H
#define MAP_E2_NODE_SOCKADDR_H 

#include "../lib/e2ap/e2ap_global_node_id_wrapper.h"
#include "../util/alg_ds/ds/assoc_container/assoc_generic.h"
#include "../util/alg_ds/ds/assoc_container/bimap.h"


#include "../lib/ep/sctp_msg.h"

#include <netinet/in.h>
#include <pthread.h>

typedef struct{
//  assoc_rb_tree_t tree; // key: global_e2_node_id_t | value: sctp_info_t     

  bi_map_t map; // left  key: global_e2_node_id_t | value: sctp_info_t 
                // right key: sctp_info_t | value: global_e2_node_id_t 

  pthread_mutex_t mtx;
} map_e2_node_sockaddr_t ; //


void init_map_e2_node_sad(map_e2_node_sockaddr_t* m);

void free_map_e2_node_sad(map_e2_node_sockaddr_t* m);

void add_map_e2_node_sad(map_e2_node_sockaddr_t* m, global_e2_node_id_t const* id, sctp_info_t const* info );

//void rm_map_e2_node_sad(map_e2_node_sockaddr_t* m, global_e2_node_id_t* id);

global_e2_node_id_t* rm_map_sad_e2_node(map_e2_node_sockaddr_t* m, sctp_info_t const* s);

sctp_info_t find_map_e2_node_sad(map_e2_node_sockaddr_t* m, global_e2_node_id_t const* id);

sctp_info_t find_map_e2_node_sad(map_e2_node_sockaddr_t * m, global_e2_node_id_t const* id);

#endif

