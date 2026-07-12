/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2_NODE_CONNECTED_XAPP_H
#define E2_NODE_CONNECTED_XAPP_H

#include <stdbool.h>
#include "../lib/e2ap/e2ap_global_node_id_wrapper.h"
#include "../lib/e2ap/e2ap_node_component_config_add_wrapper.h"
#include "../lib/e2ap/e2ap_node_component_config_add_wrapper.h"
#include "sm_ran_function.h"

typedef struct{
  global_e2_node_id_t id;

#ifdef E2AP_V1
#elif defined(E2AP_V2) || defined(E2AP_V3)
  // [1-256]
  uint8_t len_cca; // one less than in the standard, but should be OK
  e2ap_node_component_config_add_t* cca;
#endif

  // Decoded RAN Function ran_function_t
  size_t len_rf;
  sm_ran_function_t* rf;
} e2_node_connected_xapp_t;

void free_e2_node_connected_xapp(e2_node_connected_xapp_t* src);

e2_node_connected_xapp_t cp_e2_node_connected_xapp(e2_node_connected_xapp_t const* src);

bool eq_e2_node_connected_xapp(e2_node_connected_xapp_t const* m0, e2_node_connected_xapp_t const* m1);

#endif

