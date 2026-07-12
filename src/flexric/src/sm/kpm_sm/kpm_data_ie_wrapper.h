/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef KPM_DATA_IE_WRAPPER_H
#define KPM_DATA_IE_WRAPPER_H 

#ifdef KPM_V2_01
#include "kpm_sm_v02.01/ie/kpm_data_ie.h"
#elif defined(KPM_V2_03)
#include "kpm_sm_v02.03/ie/kpm_data_ie.h"
#elif defined(KPM_V3_00)
#include "kpm_sm_v03.00/ie/kpm_data_ie.h"
#else
_Static_assert(0!=0, "Unknown KPM version");
#endif

#endif
