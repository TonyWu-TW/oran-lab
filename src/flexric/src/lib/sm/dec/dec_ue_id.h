/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef DECODING_UE_ID_H
#define DECODING_UE_ID_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../ie/ue_id.h"
#include "../../../sm/rc_sm/ie/asn/UEID.h"

ue_id_e2sm_t dec_ue_id_asn(const UEID_t * ue_id_asn);

#ifdef __cplusplus
}
#endif

#endif
