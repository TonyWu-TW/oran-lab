/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef GLOBAL_CONST_WRAPPER_MIR_H
#define GLOBAL_CONST_WRAPPER_MIR_H 

#ifdef E2AP_V1
#include "v1_01/global_consts.h"
#elif E2AP_V2 
#include "v2_03/global_consts.h"
#elif E2AP_V3 
#include "v3_01/global_consts.h"
#else
static_assert(0!=0, "Unknown E2AP version");
#endif

#endif


