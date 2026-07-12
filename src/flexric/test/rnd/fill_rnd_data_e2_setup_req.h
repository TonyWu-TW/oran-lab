/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "../../src/lib/e2ap/e2ap_node_component_config_add_wrapper.h"

#ifdef E2AP_V1

#elif defined(E2AP_V2) || defined(E2AP_V3) 

e2ap_node_component_config_add_t fill_ngap_e2ap_node_component_config_add(void);

e2ap_node_component_config_add_t fill_f1ap_e2ap_node_component_config_add(void);
  
e2ap_node_component_config_add_t fill_e1ap_e2ap_node_component_config_add(void);

e2ap_node_component_config_add_t fill_s1ap_e2ap_node_component_config_add(void);

#else
static_assert(0!=0, "Unknown E2AP version");
#endif






