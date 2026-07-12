/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "ric_service_update_failure.h"
#include <stddef.h>

bool eq_ric_service_update_failure(const ric_service_update_failure_t* m0, const ric_service_update_failure_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->len_rej != m1->len_rej) 
    return false;

  for(size_t i = 0; i < m0->len_rej; ++i){
    if(eq_rejected_ran_function(&m0->rejected[i], &m1->rejected[i]) == false)
      return false;
  }

  if(eq_time_to_wait(m0->time_to_wait, m1->time_to_wait) == false)
    return false;

  if(eq_criticality_diagnostics(m0->crit_diag, m1->crit_diag ) == false)
    return false;

  return true;
}

