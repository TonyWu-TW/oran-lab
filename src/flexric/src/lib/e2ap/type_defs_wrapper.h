/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef TYPEDEFS_WRAPPER_E2AP_MIR_H
#define TYPEDEFS_WRAPPER_E2AP_MIR_H 

#ifdef E2AP_V1
#include "v1_01/type_defs.h"
#elif E2AP_V2
#include "v2_03/type_defs.h"
#elif E2AP_V3
#include "v3_01/type_defs.h"
#else
static_assert(0!=0, "Unknown E2AP version ");
#endif

#endif
