/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "ric_subscription_delete_response.h"

#include <stddef.h>

bool eq_ric_subscription_delete_response(const ric_subscription_delete_response_t* m0, const ric_subscription_delete_response_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(eq_ric_gen_id(&m0->ric_id, &m1->ric_id) == false)
    return false;

  return true;
}

