/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2_SETUP_RESPONSE_H
#define E2_SETUP_RESPONSE_H

#include "common/e2ap_global_ric_id.h"
#include "common/e2ap_node_component_config_update.h"
#include "common/e2ap_rejected_ran_function.h"
#include <stdbool.h>

typedef uint16_t accepted_ran_function_t;

typedef struct {
  global_ric_id_t id;

  accepted_ran_function_t* accepted;
  size_t len_acc;

  rejected_ran_function_t* rejected;
  size_t len_rej;
  e2_node_component_config_update_t* comp_conf_update_ack_list;
  size_t len_ccual;
} e2_setup_response_t;

bool eq_e2_setup_response(const e2_setup_response_t* m0, const e2_setup_response_t* m1);

#endif

