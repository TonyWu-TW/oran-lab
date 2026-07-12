/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E2_REMOVAL_FAILURE_V2_MIR_H
#define E2_REMOVAL_FAILURE_V2_MIR_H 

#include <stdint.h>
#include "common/e2ap_cause.h"
#include "common/e2ap_criticality_diagnostics.h"

typedef struct{

  // Transaction ID
  // 9.2.33
  // Mandatory
  uint8_t trans_id;

  //Cause
  //9.2.1
  // Mandatory
  cause_t cause;

  //Criticality Diagnostics
  //9.2.2
  //Optional
  criticality_diagnostics_t* crit;

} e2_removal_failure_t;

#endif
