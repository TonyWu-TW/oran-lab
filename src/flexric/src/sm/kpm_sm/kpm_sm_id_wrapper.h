/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef KPM_SM_ID_WRAPPER_H 
#define KPM_SM_ID_WRAPPER_H 

#ifdef KPM_V2_01
#include "kpm_sm_v02.01/kpm_sm_id.h"
#elif defined(KPM_V2_03)
#include "kpm_sm_v02.03/kpm_sm_id.h"
#elif defined(KPM_V3_00)
#include "kpm_sm_v03.00/kpm_sm_id.h"
#else
_Static_assert(0!=0, "Unknown KPM version");
#endif

#endif


