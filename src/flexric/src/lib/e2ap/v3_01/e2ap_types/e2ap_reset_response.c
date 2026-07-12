/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "e2ap_reset_response.h"


bool eq_reset_response(const e2ap_reset_response_t* m0, const e2ap_reset_response_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->trans_id != m1->trans_id)
    return false;

  if(eq_criticality_diagnostics(m0->crit_diag, m1->crit_diag) == false)
    return false;

  return true;
}

