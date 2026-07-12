/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "e2ap_rejected_ran_function.h"

#include <stddef.h>


bool eq_rejected_ran_function(const rejected_ran_function_t* m0, const rejected_ran_function_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->id != m1->id)
    return false;

  if(eq_cause(&m0->cause, &m1->cause) == false)
    return false;

  return true;
}

