/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef DECODING_NG_ENB_H
#define DECODING_NG_ENB_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../ie/ng_enb.h"
#include "dec_asn.h"

ng_enb_e2sm_t dec_ng_eNB_UE_asn(const UEID_NG_ENB_t * ng_enb_asn);

#ifdef __cplusplus
}
#endif

#endif
