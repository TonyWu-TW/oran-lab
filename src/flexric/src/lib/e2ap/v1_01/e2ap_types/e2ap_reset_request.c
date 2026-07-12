/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "e2ap_reset_request.h"
#include <stddef.h>

bool eq_reset_request(const e2ap_reset_request_t* m0, const e2ap_reset_request_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(eq_cause(&m0->cause, &m1->cause) == false)
    return false;

  return true;
}

