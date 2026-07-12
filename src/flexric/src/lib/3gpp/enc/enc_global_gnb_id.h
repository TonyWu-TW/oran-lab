/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef ENCODING_GLOBAL_GNB_ID_H
#define ENCODING_GLOBAL_GNB_ID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../ie/global_gnb_id.h"
#include "enc_asn.h"

GlobalGNB_ID_t * enc_global_gnb_id_asn(const global_gnb_id_t * global_gnb_id);

#ifdef __cplusplus
}
#endif

#endif
