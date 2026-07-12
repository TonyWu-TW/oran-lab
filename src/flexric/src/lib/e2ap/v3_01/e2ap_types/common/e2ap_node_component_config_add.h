/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_NODE_COMPONENT_CONFIG_ADD_H
#define E2AP_NODE_COMPONENT_CONFIG_ADD_H

#include "e2ap_node_comp_interface_type.h"
#include "e2ap_node_comp_id.h"
#include "e2ap_node_comp_conf.h"

// From 9.1.2.2 E2 SETUP REQUEST
typedef struct{
  // Mandatory
  // 9.2.26
  e2ap_node_comp_interface_type_e e2_node_comp_interface_type;

  // Bug in the standard!!! Optional in the standard, Mandatory in asn specs
  // Let's take the asn definition;
  // 9.2.32
  e2ap_node_comp_id_t e2_node_comp_id;

  // Mandatory
  // 9.2.27
  e2ap_node_comp_conf_t e2_node_comp_conf;

} e2ap_node_component_config_add_t;

void free_e2ap_node_component_config_add(e2ap_node_component_config_add_t* src);

e2ap_node_component_config_add_t cp_e2ap_node_component_config_add(e2ap_node_component_config_add_t const* src);

bool eq_e2ap_node_component_config_add(e2ap_node_component_config_add_t const* m0, e2ap_node_component_config_add_t const* m1);

typedef struct{
  e2ap_node_component_config_add_t* cca;
  size_t len_cca; 
} arr_node_component_config_add_t ; 

#endif

