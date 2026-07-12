/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef DECODING_ENB_H
#define DECODING_ENB_H


#ifdef __cplusplus
extern "C" {
#endif

#include "dec_asn.h"
#include "../ie/enb.h"

enb_e2sm_t dec_eNB_UE_asn(const UEID_ENB_t * enb_asn);

#ifdef __cplusplus
}
#endif

#endif
