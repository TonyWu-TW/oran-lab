/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef DECODING_GLOBAL_NG_ENB_H
#define DECODING_GLOBAL_NG_ENB_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../ie/global_ng_enb_id.h"
#include "dec_asn.h"

global_ng_enb_id_t dec_global_ng_enb_asn(const GlobalNgENB_ID_t * global_ng_enb_id_asn);

#ifdef __cplusplus
}
#endif

#endif
