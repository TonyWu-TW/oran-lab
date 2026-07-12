/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "ric_subscription_failure.h"

bool eq_ric_subscritption_failure(const ric_subscription_failure_t* m0, const ric_subscription_failure_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(eq_ric_gen_id(&m0->ric_id, &m1->ric_id) == false)
    return false;

  if(eq_cause(&m0->cause, &m1->cause ) == false)
    return false;

  if(eq_criticality_diagnostics(m0->crit_diag, m1->crit_diag) == false)
    return false;

  return true;
}

