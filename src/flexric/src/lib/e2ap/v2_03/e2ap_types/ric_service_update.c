/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "ric_service_update.h"

bool eq_ric_service_update(const ric_service_update_t* m0, const ric_service_update_t* m1)
{

  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->trans_id != m1->trans_id)
    return false;

  if(m0->len_added != m1->len_added)
    return false;

  for(size_t i = 0; i < m0->len_added; ++i){
    if(eq_ran_function(&m0->added[i], &m1->added[i]) == false)
      return false;
  }

  if(m0->len_modified != m1->len_modified)
    return false;

  for(size_t i = 0; i < m0->len_modified; ++i){
    if(eq_ran_function(&m0->modified[i], &m1->modified[i]))
      return false;
  }

  if(m0->len_deleted != m1->len_deleted)
    return false;

  for(size_t i = 0; i < m0->len_deleted; ++i){
    if(eq_ran_function_id_rev(&m0->deleted[i], &m1->deleted[i]) == false)
      return false;
  }
  return true;
}
