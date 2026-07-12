/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_AP_WRAPPER_MIR_H
#define E2AP_AP_WRAPPER_MIR_H 

#ifdef E2AP_V1
#include "v1_01/e2ap_ap.h"
#elif E2AP_V2
#include "v2_03/e2ap_ap.h"
#elif E2AP_V3
#include "v3_01/e2ap_ap.h"
#else
static_assert(0!=0, "Unknown E2AP Version");
#endif




#endif

