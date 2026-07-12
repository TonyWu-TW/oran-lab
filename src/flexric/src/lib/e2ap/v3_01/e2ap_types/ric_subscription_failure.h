/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_SUBSCRIPTION_FAILURE_H
#define RIC_SUBSCRIPTION_FAILURE_H

#include <stddef.h>

#include "common/ric_gen_id.h"
#include "common/ric_action_not_admitted.h"
#include "common/e2ap_criticality_diagnostics.h"


typedef struct {
  // Mandatory
  ric_gen_id_t ric_id;
  // Mandatory
  cause_t cause;
  // Optional 
  criticality_diagnostics_t* crit_diag; 

} ric_subscription_failure_t;


bool eq_ric_subscritption_failure(const ric_subscription_failure_t* m0, const ric_subscription_failure_t* m1);

#endif
