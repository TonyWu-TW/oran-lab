/*
 *
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "e2_setup_response.h"



bool eq_e2_setup_response(const e2_setup_response_t* m0, const e2_setup_response_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(eq_global_ric_id(&m0->id, &m1->id) == false )
    return false;

  if(m0->len_acc != m1->len_acc) 
    return false;

  for(size_t i = 0; i < m0->len_acc; ++i){
    if(m0->accepted[i] != m1->accepted[i])
      return false;
  }

  if(m0->len_rej != m1->len_rej)
    return false;

  for(size_t i = 0 ; i < m0->len_rej; ++i){
    if(eq_rejected_ran_function(&m0->rejected[i], &m1->rejected[i]) == false)
      return false;
  }

  if(m0->len_ccual != m1->len_ccual)
    return false;

  for(size_t i = 0 ; i < m0->len_ccual; ++i){
    if(eq_e2_node_component_config_update(m0->comp_conf_update_ack_list, m1->comp_conf_update_ack_list) == false)
      return false;
  }

  return true;
}
