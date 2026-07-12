/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2_SETUP_RESPONSE_H
#define E2_SETUP_RESPONSE_H

#include "common/e2ap_global_ric_id.h"
#include "common/e2ap_node_comp_config_add_ack.h"
#include "common/e2ap_rejected_ran_function.h"
#include "common/e2ap_rejected_ran_function.h"
#include <stdbool.h>
#include <stdlib.h>

typedef uint16_t accepted_ran_function_t;

typedef struct {

  // Mandatory
  uint8_t trans_id;

  // Mandatory
  global_ric_id_t id;

  // [0-256]
  accepted_ran_function_t* accepted;
  size_t len_acc;

  // [0-256]
  rejected_ran_function_t* rejected;
  size_t len_rej;

  // [1-1024]
  e2ap_node_comp_config_add_ack_t* comp_config_add_ack;
  size_t len_ccaa;

} e2_setup_response_t;

void free_e2_setup_response(e2_setup_response_t* src);

bool eq_e2_setup_response(const e2_setup_response_t* m0, const e2_setup_response_t* m1);

#endif
