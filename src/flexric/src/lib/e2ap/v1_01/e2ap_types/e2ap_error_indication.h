/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2AP_ERROR_INDICATION_H
#define E2AP_ERROR_INDICATION_H

#include "common/ric_gen_id.h"
#include "common/e2ap_cause.h"
#include "common/e2ap_criticality_diagnostics.h"

typedef struct {
  ric_gen_id_t* ric_id; // optional
  cause_t* cause; // optional
  criticality_diagnostics_t* crit_diag; // optional
} e2ap_error_indication_t;

bool eq_error_indication(const e2ap_error_indication_t* m0, const e2ap_error_indication_t* m1);

#endif

