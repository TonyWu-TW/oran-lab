/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "ric_service_update_ack.h"

bool eq_ric_service_update_ack(const ric_service_update_ack_t* m0, const  ric_service_update_ack_t* m1)
{

  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->len_accepted != m1->len_accepted)
    return false;

  for(size_t i = 0; i < m0->len_accepted; ++i){
    if(eq_ran_function_id(&m0->accepted[i], &m1->accepted[i]) == false)
      return false;
  }

  if(m0->len_rejected != m1->len_rejected)
    return false;

  for(size_t i = 0; i < m0->len_rejected; ++i){
    if(eq_rejected_ran_function(&m0->rejected[i], &m1->rejected[i]) == false)
      return false;
  }

  return true;
}

