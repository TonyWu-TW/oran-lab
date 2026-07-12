/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_SUBSCRIPTION_DELETE_FAILURE_H
#define RIC_SUBSCRIPTION_DELETE_FAILURE_H

#include "common/ric_gen_id.h"
#include "common/e2ap_cause.h"
#include "common/e2ap_criticality_diagnostics.h"

#include <stdbool.h>

typedef struct{
  ric_gen_id_t ric_id;
  cause_t cause;
  criticality_diagnostics_t* crit_diag; // optional
} ric_subscription_delete_failure_t;

bool eq_ric_subscription_delete_failure(const ric_subscription_delete_failure_t* m0, const  ric_subscription_delete_failure_t* m1);

#endif


