/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_NODE_COMP_CONF_ADD_ACK_MIR_H
#define E2AP_NODE_COMP_CONF_ADD_ACK_MIR_H 

#include "e2ap_cause.h"
#include <stdbool.h>

typedef enum{
  SUCCESS_E2AP_NODE_COMP_CONF_ACK = 0,
  FAILURE_E2AP_NODE_COMP_CONF_ACK,
} outcome_e2ap_node_comp_conf_ack_e;


// 9.2.28
// Mandatory
typedef struct {

  // Mandatory
  // Outcome
  outcome_e2ap_node_comp_conf_ack_e outcome;

  // Optional
  // 9.2.1
  cause_t* cause;

} e2ap_node_comp_conf_ack_t;

void free_e2ap_node_comp_conf_ack( e2ap_node_comp_conf_ack_t* src);

e2ap_node_comp_conf_ack_t cp_e2ap_node_comp_conf_ack( e2ap_node_comp_conf_ack_t const* src);

bool eq_e2ap_node_comp_conf_ack( e2ap_node_comp_conf_ack_t const* m0, e2ap_node_comp_conf_ack_t const* m1);

#endif
