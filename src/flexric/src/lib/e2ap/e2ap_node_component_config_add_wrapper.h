/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_NODE_COMPONENT_CONFIG_ADD_WRAPPER_H
#define E2AP_NODE_COMPONENT_CONFIG_ADD_WRAPPER_H

#ifdef E2AP_V1

#elif E2AP_V2 
#include "v2_03/e2ap_types/common/e2ap_node_component_config_add.h"
#elif E2AP_V3 
#include "v3_01/e2ap_types/common/e2ap_node_component_config_add.h"
#else
static_assert(0!=0, "Unknown E2AP version");
#endif




#endif
