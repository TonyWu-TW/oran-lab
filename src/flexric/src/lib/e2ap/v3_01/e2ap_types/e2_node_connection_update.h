/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2_NODE_CONNECTION_UPDATE_H
#define E2_NODE_CONNECTION_UPDATE_H

#include "common/e2ap_connection_update_item.h"
#include "common/e2ap_connection_update_remove_item.h"

typedef struct{
  e2_connection_update_item_t* add;
  size_t len_add;
  e2_connection_update_remove_item_t* rem;
  size_t len_rem;
  e2_connection_update_item_t* mod;
  size_t len_mod;
} e2_node_connection_update_t;

#endif

