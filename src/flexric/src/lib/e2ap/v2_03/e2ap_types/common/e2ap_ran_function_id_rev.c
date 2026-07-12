/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "e2ap_ran_function_id_rev.h"
#include <stddef.h>

bool eq_ran_function_id_rev(const e2ap_ran_function_id_rev_t* m0, const e2ap_ran_function_id_rev_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->id != m1->id)
    return false;

  if(m0->rev != m1->rev)
    return false;

  return true;
}


