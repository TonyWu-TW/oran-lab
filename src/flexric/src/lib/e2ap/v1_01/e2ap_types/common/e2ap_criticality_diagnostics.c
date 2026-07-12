/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "e2ap_criticality_diagnostics.h"
#include <assert.h>

bool eq_criticality_diagnostics(const criticality_diagnostics_t* m0, const criticality_diagnostics_t* m1)
{
  if(m0 == m1) return true;

  if(m0 != NULL || m1 != NULL) return false;

  assert(0!=0 && "Not implemented");

  return true;
}

