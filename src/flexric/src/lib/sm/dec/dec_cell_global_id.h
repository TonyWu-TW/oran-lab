/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef DECODING_CELL_GLOBAL_ID_H
#define DECODING_CELL_GLOBAL_ID_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../../../sm/rc_sm/ie/asn/CGI.h"
#include "../ie/cell_global_id.h"

cell_global_id_t dec_cell_global_id_asn(const CGI_t * cell_global_id_asn);

#ifdef __cplusplus
}
#endif

#endif
