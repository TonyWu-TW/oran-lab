/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2_NODE_CONNECTED
#define E2_NODE_CONNECTED 

#include "common/e2ap_global_node_id.h"
#include "common/e2ap_node_component_config_add.h"
#include "common/e2ap_ran_function.h"

#include <stdbool.h>

typedef struct{
  global_e2_node_id_t id;

  // [1-256]
  uint8_t len_cca; // one less than in the standard, but should be OK
  e2ap_node_component_config_add_t* cca;

  size_t len_rf;
  ran_function_t* ack_rf;
} e2_node_connected_t;

e2_node_connected_t cp_e2_node_connected(const e2_node_connected_t* src);

void free_e2_node_connected(e2_node_connected_t* src);

bool eq_e2_node_connected(const e2_node_connected_t* m0, const e2_node_connected_t* m1);

#endif

