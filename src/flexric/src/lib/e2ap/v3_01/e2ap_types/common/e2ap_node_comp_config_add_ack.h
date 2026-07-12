/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_NODE_COMP_CONFIG_ADD_ACK_MIR_H
#define E2AP_NODE_COMP_CONFIG_ADD_ACK_MIR_H

#include "e2ap_node_comp_interface_type.h"
#include "e2ap_node_comp_id.h"
#include "e2ap_node_comp_conf_add_ack.h"

typedef struct{
  // 9.2.26
  // Mandatory
  e2ap_node_comp_interface_type_e e2_node_comp_interface_type;

  // 9.2.32
  // Mandatory
  e2ap_node_comp_id_t e2_node_comp_id;

  // 9.2.28
  // Mandatory
  e2ap_node_comp_conf_ack_t e2_node_comp_conf_ack;

} e2ap_node_comp_config_add_ack_t;

void free_e2ap_node_comp_config_add_ack(e2ap_node_comp_config_add_ack_t* src);

e2ap_node_comp_config_add_ack_t cp_e2ap_node_comp_config_add_ack(e2ap_node_comp_config_add_ack_t const* src);

bool eq_e2ap_node_component_config_add_ack(e2ap_node_comp_config_add_ack_t const* m0, e2ap_node_comp_config_add_ack_t const* m1);

#endif
