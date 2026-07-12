/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2_SETUP_FAILURE_H
#define E2_SETUP_FAILURE_H

#include "common/e2ap_cause.h"
#include "common/e2ap_criticality_diagnostics.h"
#include "common/e2ap_time_to_wait.h"
#include "common/transport_layer_info.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  cause_t cause;
  e2ap_time_to_wait_e* time_to_wait_ms;       // optional
  criticality_diagnostics_t* crit_diag;   // optional
  transport_layer_information_t* tl_info; // optional
} e2_setup_failure_t;

bool eq_e2_setup_failure(const e2_setup_failure_t* m0, const e2_setup_failure_t* m1);

#endif

