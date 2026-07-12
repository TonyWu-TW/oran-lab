/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef ENCODING_EN_GNB_H
#define ENCODING_EN_GNB_H


#ifdef __cplusplus
extern "C" {
#endif

#include "enc_asn.h"
#include "../ie/en_gnb.h"

UEID_EN_GNB_t * enc_en_gNB_UE_asn(const en_gnb_e2sm_t * en_gnb);

#ifdef __cplusplus
}
#endif

#endif
