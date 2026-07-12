/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2_REMOVAL_RESPONSE_E2AP_V2_H
#define E2_REMOVAL_RESPONSE_E2AP_V2_H

#include <stdint.h>
#include "common/e2ap_criticality_diagnostics.h"

typedef struct{
  // 9.2.33
  // Mandatory
  uint8_t trans_id;

  // 9.2.2
  // Optional
  criticality_diagnostics_t* crit;

}e2_removal_response_t;

#endif
