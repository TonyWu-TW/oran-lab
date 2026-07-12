/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E42_SETUP_REQUEST_REQUEST_WRAPPER_MIR_H
#define E42_SETUP_REQUEST_REQUEST_WRAPPER_MIR_H

#ifdef E2AP_V1
#include "v1_01/e2ap_types/e42_setup_request.h"
#elif E2AP_V2
#include "v2_03/e2ap_types/e42_setup_request.h"
#elif E2AP_V3
#include "v3_01/e2ap_types/e42_setup_request.h"
#else
static_assert(0!=0 , "Not implemented");
#endif

#endif

