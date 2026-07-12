/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include <assert.h>
#include "e2_setup_failure.h"

bool eq_e2_setup_failure(const e2_setup_failure_t* m0, const e2_setup_failure_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->trans_id != m1->trans_id){
    assert(0!=0 && "Debugging");
    return false;
  }

  if(eq_cause(&m0->cause, &m1->cause) == false)
    return false;

  if(eq_time_to_wait(m0->time_to_wait_ms, m1->time_to_wait_ms) == false)
    return false;

  if(eq_criticality_diagnostics(m0->crit_diag, m1->crit_diag) == false)
    return false;

  if(eq_transport_layer_information(m0->tl_info, m1->tl_info) == false)
    return false;

  return true;
}
