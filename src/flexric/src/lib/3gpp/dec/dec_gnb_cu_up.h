/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef DECODING_GNB_CU_UP_H
#define DECODING_GNB_CU_UP_H


#ifdef __cplusplus
extern "C" {
#endif

#include "dec_asn.h"
#include "../ie/gnb_cu_up.h"

gnb_cu_up_e2sm_t dec_gNB_CU_UP_UE_asn(const UEID_GNB_CU_UP_t * gnb_cu_up_asn);

#ifdef __cplusplus
}
#endif

#endif
