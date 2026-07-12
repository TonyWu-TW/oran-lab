/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2_node_connected.h"

#include <assert.h>
#include <stdlib.h>

e2_node_connected_t cp_e2_node_connected(const e2_node_connected_t* src)
{
  assert(src != NULL);

  e2_node_connected_t dst = {0};

  dst.id = cp_global_e2_node_id(&src->id);

  // [1-256]
  assert(src->len_cca > 0);
  dst.cca = calloc(src->len_cca, sizeof(e2ap_node_component_config_add_t));
  assert(dst.cca != NULL && "Memory exhausted");
  for(size_t i = 0; i < dst.len_cca; ++i){
    dst.cca[i] = cp_e2ap_node_component_config_add(&src->cca[i]);
  }

  dst.len_rf = src->len_rf;
  if(dst.len_rf > 0){
    dst.ack_rf = calloc(dst.len_rf, sizeof(ran_function_t));
    assert(dst.ack_rf != NULL && "Memory exhausted");
  }

  for(size_t i =0; i < dst.len_rf; ++i){
    dst.ack_rf[i] = cp_ran_function( &src->ack_rf[i]); 
  }
  return dst;
}

void free_e2_node_connected(e2_node_connected_t* src)
{
  assert(src != NULL);

  free_global_e2_node_id(&src->id);

  assert(src->len_cca > 0);
  for(size_t i = 0; i < src->len_cca; ++i){
     free_e2ap_node_component_config_add(&src->cca[i]);
  }
  free(src->cca);

  for(size_t i = 0; i < src->len_rf; ++i){
    ran_function_t* rf = &src->ack_rf[i]; 
    free_ran_function(rf);
  }
  free(src->ack_rf);
}

bool eq_e2_node_connected(const e2_node_connected_t* m0, const e2_node_connected_t* m1)
{
  if(m0 == m1)
    return true;

  if(m0 == NULL)
    return false;

  if(m1 == NULL)
    return false;

  if( eq_global_e2_node_id(&m0->id, &m1->id) == false)
    return false;
 
  // [1 - 255 ]
  assert(m0->len_cca > 0 && m1->len_cca > 0);
  if(m0->len_cca != m1->len_cca)
    return false;
  for(size_t i = 0; i < m0->len_cca; ++i){
    if(eq_e2ap_node_component_config_add(&m0->cca[i], &m1->cca[i]) == false)
      return false;
  }

  if(m0->len_rf != m1->len_rf)
    return false;

  for(size_t i = 0; i < m0->len_rf; ++i){
    if(eq_ran_function(&m0->ack_rf[i], &m1->ack_rf[i] ) == false)
      return false;
  }

  return true;
}

