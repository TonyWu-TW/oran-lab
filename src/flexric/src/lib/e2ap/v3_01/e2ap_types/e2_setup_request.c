/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2_setup_request.h"

#include <assert.h>
#include <stdlib.h>

e2_setup_request_t cp_e2_setup_request(const e2_setup_request_t* src)
{
  assert(src != NULL);
  e2_setup_request_t dst = {.trans_id = src->trans_id,.id = src->id, .len_rf = src->len_rf, .len_cca = src->len_cca }; 

  dst.ran_func_item = calloc(src->len_rf, sizeof(ran_function_t));
  assert(dst.ran_func_item != NULL && "Memory exhausted");

  for(size_t i = 0; i < src->len_rf; ++i){
    dst.ran_func_item[i] = cp_ran_function(&src->ran_func_item[i]); 
  }

  dst.comp_conf_add = calloc(src->len_cca, sizeof(e2ap_node_component_config_add_t) ); 
  assert(dst.comp_conf_add != NULL && "Memory exhausted");

  for(size_t i = 0; i < src->len_cca; ++i){
    dst.comp_conf_add[i] = cp_e2ap_node_component_config_add(&src->comp_conf_add[i]); 
  }

  return dst;
}

void free_e2_setup_request(e2_setup_request_t* src)
{
  assert(src != NULL);

  free_global_e2_node_id(&src->id);

  for(size_t i = 0; i < src->len_rf; ++i){
    free_ran_function(&src->ran_func_item[i]); 
  }
  free(src->ran_func_item);

  for(size_t i = 0; i < src->len_cca; ++i){
   free_e2ap_node_component_config_add(&src->comp_conf_add[i]); 
  }
  free(src->comp_conf_add);
}

bool eq_e2_setup_request(const e2_setup_request_t* m0, const e2_setup_request_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(m0->trans_id != m1->trans_id)
    return false;

  if(eq_global_e2_node_id(&m0->id,&m1->id) == false)
    return false;

  if(m0->len_rf != m1->len_rf)
    return false;

  for(size_t i = 0; i < m0->len_rf; ++i){
    if(eq_ran_function(&m0->ran_func_item[i], &m1->ran_func_item[i]) == false)
      return false;
  }

  if(m0->len_cca != m1->len_cca)
    return false;

  for(size_t i = 0; i < m0->len_cca; ++i){
    if(eq_e2ap_node_component_config_add(&m0->comp_conf_add[i], &m1->comp_conf_add[i]) == false) 
      return false;
  }

  return true;
}

