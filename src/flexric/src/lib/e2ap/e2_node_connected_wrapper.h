/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2_NODE_CONNECTED_WRAPPER_MIR_H
#define E2_NODE_CONNECTED_WRAPPER_MIR_H 

#ifdef E2AP_V1
#include "v1_01/e2ap_types/e2_node_connected.h"
#elif E2AP_V2 
#include "v2_03/e2ap_types/e2_node_connected.h"
#elif E2AP_V3 
#include "v3_01/e2ap_types/e2_node_connected.h"
#else 
static_assert(0!=0, "Unknwon E2AP Version");
#endif

#endif
