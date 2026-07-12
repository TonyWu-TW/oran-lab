/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2_SETUP_REQUEST_H
#define E2_SETUP_REQUEST_H

#include <stddef.h>
#include "common/e2ap_ran_function.h"
#include "common/e2ap_global_node_id.h"
#include "common/e2ap_node_component_config_add.h"

typedef struct e2_setup_request {
  uint8_t trans_id; 
  global_e2_node_id_t id;
  ran_function_t* ran_func_item;
  size_t len_rf;
  e2ap_node_component_config_add_t* comp_conf_add;
  size_t len_cca;
} e2_setup_request_t;

e2_setup_request_t cp_e2_setup_request(const e2_setup_request_t* src);

void free_e2_setup_request(e2_setup_request_t* src);

bool eq_e2_setup_request(const e2_setup_request_t* m0, const e2_setup_request_t* m1);

#endif

