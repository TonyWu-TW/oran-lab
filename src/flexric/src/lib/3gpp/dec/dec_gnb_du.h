/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef DECODING_GNB_DU_H
#define DECODING_GNB_DU_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../ie/gnb_du.h"
#include "dec_asn.h"

gnb_du_e2sm_t dec_gNB_DU_UE_asn(const UEID_GNB_DU_t * gnb_du_asn);

#ifdef __cplusplus
}
#endif

#endif
