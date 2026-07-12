/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef ENCODING_ENB_H
#define ENCODING_ENB_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../ie/enb.h"
#include "enc_asn.h"

UEID_ENB_t * enc_eNB_UE_asn(const enb_e2sm_t * enb);

#ifdef __cplusplus
}
#endif

#endif
