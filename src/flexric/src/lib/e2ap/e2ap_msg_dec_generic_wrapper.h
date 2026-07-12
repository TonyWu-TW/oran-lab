/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2AP_MSG_DECODE_GENERIC_WRAPPER_MIR_H
#define E2AP_MSG_DECODE_GENERIC_WRAPPER_MIR_H 


#ifdef E2AP_V1
#include "v1_01/dec/e2ap_msg_dec_generic.h"
#elif E2AP_V2 
#include "v2_03/dec/e2ap_msg_dec_generic.h"
#elif E2AP_V3 
#include "v3_01/dec/e2ap_msg_dec_generic.h"
#else
static_assert(0!=0, "Unknown E2AP version");
#endif

#endif
