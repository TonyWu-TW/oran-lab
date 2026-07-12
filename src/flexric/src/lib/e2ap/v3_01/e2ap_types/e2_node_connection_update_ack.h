/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2_NODE_CONNECTION_UPDATE_ACK_H
#define E2_NODE_CONNECTION_UPDATE_ACK_H 

#include "common/e2ap_connection_update_item.h"
#include "common/e2ap_connection_setup_failed.h"

typedef struct{
  e2_connection_update_item_t* setup;
  size_t len_setup;

  e2_connection_setup_failed_t* failed;
  size_t len_failed;

} e2_node_connection_update_ack_t;


#endif

