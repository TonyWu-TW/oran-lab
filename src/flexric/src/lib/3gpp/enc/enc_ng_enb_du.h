/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef ENCODING_NG_ENB_DU_H
#define ENCODING_NG_ENB_DU_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../ie/ng_enb_du.h"
#include "enc_asn.h"

UEID_NG_ENB_DU_t * enc_ng_eNB_DU_UE_asn(const ng_enb_du_e2sm_t * ng_enb_du);

#ifdef __cplusplus
}
#endif

#endif
