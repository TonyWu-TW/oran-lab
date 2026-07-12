/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "e42_ric_subscription_delete_request.h"

#include <assert.h>
#include <stdlib.h>

bool  eq_e42_ric_subscription_delete_request(const e42_ric_subscription_delete_request_t* m0, const e42_ric_subscription_delete_request_t* m1)
{
  assert(m0 != NULL);
  assert(m1 != NULL);

  if(m0->xapp_id != m1->xapp_id) 
    return false;

  return eq_ric_subscription_delete_request(&m0->sdr, &m1->sdr);
}


