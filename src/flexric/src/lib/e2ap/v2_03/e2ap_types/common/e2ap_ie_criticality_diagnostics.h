/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_IE_CRITICALITY_DIAGNOSTICS_H
#define E2AP_IE_CRITICALITY_DIAGNOSTICS_H

#include "stdint.h"
#include "e2ap_criticality.h"

typedef enum {
  NOT_UNDERSTOOD,
  MISSING 
} type_of_error_t;

typedef struct ie_criticality_diagnostics {
  criticality_e crit;
  uint16_t id;
  type_of_error_t type;
} ie_criticality_diagnostics_t;

ie_criticality_diagnostics_t copy_ie_criticality_diagnostics(const ie_criticality_diagnostics_t* src);


#endif

