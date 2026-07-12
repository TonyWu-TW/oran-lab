/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef ENCODING_UE_ID_H
#define ENCODING_UE_ID_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../../../sm/rc_sm/ie/asn/UEID.h"
#include "../ie/ue_id.h"

UEID_t enc_ue_id_asn(const ue_id_e2sm_t * ue_id);

#ifdef __cplusplus
}
#endif

#endif
