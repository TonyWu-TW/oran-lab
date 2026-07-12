/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "e2_node_configuration_update.h"


bool eq_node_configuration_update(const e2_node_configuration_update_t* m0, const e2_node_configuration_update_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->len_ccul != m1 ->len_ccul)
    return false;

  for(size_t i = 0; i < m0->len_ccul; ++i){
    if( eq_e2_node_component_config_update(&m0->comp_conf_update_list[i], &m1->comp_conf_update_list[i]) == false)
      return false;
  }
  return true;
}

