/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "e2ap_time_to_wait.h"
#include <stddef.h>

bool eq_time_to_wait(const e2ap_time_to_wait_e* m0, const e2ap_time_to_wait_e* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(*m0 < *m1)
    return false;

  return true;
}
