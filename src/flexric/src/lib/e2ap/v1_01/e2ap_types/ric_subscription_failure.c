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

  if(m0->len_na != m1->len_na)
    return false;

  for(size_t i = 0; i < m0->len_na; ++i){
    if(eq_ric_action_not_admitted(&m0->not_admitted[i], &m1->not_admitted[i]) == false){
      return false;
    }
  }

  if(eq_criticality_diagnostics(m0->crit_diag, m1->crit_diag) == false)
    return false;

  return true;
}

