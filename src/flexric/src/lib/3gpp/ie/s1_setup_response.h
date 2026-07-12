/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef S1_SETUP_RESPONSE_MIR_H
#define S1_SETUP_RESPONSE_MIR_H

#include "../../../util/byte_array.h"

// 3GPP 36.413 [24], clause 9.1.8.5
typedef struct{
  byte_array_t ba; 
} s1_setup_response_t ;

#endif
