/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2AP_RESET_RESPONSE_H
#define E2AP_RESET_RESPONSE_H

#include "common/e2ap_criticality_diagnostics.h"
#include <stdbool.h>

typedef struct {
  criticality_diagnostics_t* crit_diag; // optional
} e2ap_reset_response_t;

bool eq_reset_response(const e2ap_reset_response_t* m0, const e2ap_reset_response_t* m1);

#endif

