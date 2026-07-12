/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "e2ap_error_indication.h"


bool eq_error_indication(const e2ap_error_indication_t* m0, const e2ap_error_indication_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) 
    return false;

  if(eq_ric_gen_id(m0->ric_id, m1->ric_id) == false)
    return false;

  if(eq_cause(m0->cause, m1->cause) == false) 
    return false;

  if( eq_criticality_diagnostics(m0->crit_diag, m1->crit_diag) == false)
    return false;

  return true;
}

