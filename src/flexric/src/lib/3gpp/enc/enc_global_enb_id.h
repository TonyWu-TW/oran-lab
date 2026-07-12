/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef ENCODING_GLOBAL_ENB_ID_H
#define ENCODING_GLOBAL_ENB_ID_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../ie/global_enb_id.h"
#include "enc_asn.h"

GlobalENB_ID_t enc_global_enb_id_asn(const global_enb_id_t * global_enb_id);

#ifdef __cplusplus
}
#endif

#endif
