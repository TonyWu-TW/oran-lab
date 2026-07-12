/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef ENCODING_GNB_H
#define ENCODING_GNB_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../ie/gnb.h"
#include "enc_asn.h"

UEID_GNB_t * enc_gNB_UE_asn(const gnb_e2sm_t * gnb);

#ifdef __cplusplus
}
#endif

#endif
