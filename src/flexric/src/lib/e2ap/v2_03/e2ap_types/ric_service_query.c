/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "ric_service_query.h"


bool eq_ric_service_query(const ric_service_query_t* m0, const ric_service_query_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->trans_id != m1->trans_id)
    return false;

  if(m0->len_accepted != m1->len_accepted)
    return false;

  for(size_t i = 0; i < m0->len_accepted; ++i){
    if(eq_ran_function_id_rev(&m0->accepted[i], &m1->accepted[i]) == false)
      return false;
  }

  return true;
}

