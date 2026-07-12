/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_SUBSCRIPTION_DELETE_FAILURE_WRAPPER_MIR_H
#define RIC_SUBSCRIPTION_DELETE_FAILURE_WRAPPER_MIR_H 

#ifdef E2AP_V1
#include "v1_01/e2ap_types/ric_subscription_delete_failure.h"
#elif E2AP_V2 
#include "v2_03/e2ap_types/ric_subscription_delete_failure.h"
#elif E2AP_V3 
#include "v3_01/e2ap_types/ric_subscription_delete_failure.h"
#else
static_assert(0!=0, "Unknown E2AP version");
#endif

#endif


