/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_NODE_COMP_CONF_MIR_H
#define E2AP_NODE_COMP_CONF_MIR_H 

#include "../../../../../util/byte_array.h"
#include <stdbool.h>

// 9.2.27
// Contents depend on
// component type and used to
// carry new or updated
// component configuration. 

typedef struct{
  byte_array_t request; 
  byte_array_t response;  
} e2ap_node_comp_conf_t ;

void free_e2ap_node_comp_conf(e2ap_node_comp_conf_t* src);

bool eq_e2ap_node_comp_conf(e2ap_node_comp_conf_t const* m0, e2ap_node_comp_conf_t const* m1);

e2ap_node_comp_conf_t cp_e2ap_node_comp_conf(e2ap_node_comp_conf_t const* src);

#endif

