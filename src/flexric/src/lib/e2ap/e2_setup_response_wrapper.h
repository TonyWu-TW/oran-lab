/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2_SETUP_RESPONSE_WRAPPER_MIR_H
#define E2_SETUP_RESPONSE_WRAPPER_MIR_H

#ifdef E2AP_V1
#include "v1_01/e2ap_types/e2_setup_response.h"                // for e2_se...
#elif E2AP_V2
#include "v2_03/e2ap_types/e2_setup_response.h"                // for e2_se...
#elif E2AP_V3
#include "v3_01/e2ap_types/e2_setup_response.h"                // for e2_se...
#else
static_assert(0!=0, "Unknown E2AP version");
#endif

//
#endif
