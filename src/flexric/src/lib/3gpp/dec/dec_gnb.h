/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef DECODING_GNB_H
#define DECODING_GNB_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../ie/gnb.h"
#include "dec_asn.h"

gnb_e2sm_t dec_gNB_UE_asn(const UEID_GNB_t * gnb_asn);

#ifdef __cplusplus
}
#endif

#endif
