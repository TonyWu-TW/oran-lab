/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_SERVICE_UPDATE_FAILURE_H
#define RIC_SERVICE_UPDATE_FAILURE_H

#include "common/e2ap_rejected_ran_function.h"
#include "common/e2ap_criticality_diagnostics.h"
#include "common/e2ap_time_to_wait.h"

typedef struct {
  rejected_ran_function_t* rejected;
  size_t len_rej;
  e2ap_time_to_wait_e* time_to_wait;
  criticality_diagnostics_t* crit_diag;
} ric_service_update_failure_t;

bool eq_ric_service_update_failure(const ric_service_update_failure_t* m0, const ric_service_update_failure_t* m1);

#endif

