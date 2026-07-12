/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2ap_ie_criticality_diagnostics.h"

ie_criticality_diagnostics_t copy_ie_criticality_diagnostics(const ie_criticality_diagnostics_t* src)
{
  ie_criticality_diagnostics_t dst = {
    .crit = src->crit,
    .id = src->id,
    .type = src->type
  };

  return dst;
}

