/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_GEN_ID_WRAPPER_MIR_H
#define RIC_GEN_ID_WRAPPER_MIR_H 

#ifdef E2AP_V1
#include "v1_01/e2ap_types/common/ric_gen_id.h"
#elif E2AP_V2
#include "v2_03/e2ap_types/common/ric_gen_id.h"
#elif E2AP_V3
#include "v3_01/e2ap_types/common/ric_gen_id.h"
#else
static_assert(0!=0, "Unkown E2AP version");
#endif

#endif

