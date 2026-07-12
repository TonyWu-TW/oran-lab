
/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "e42_setup_response.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

e42_setup_response_t cp_e42_setup_response(const e42_setup_response_t* src)
{
  assert(src != NULL);

  e42_setup_response_t dst = {.xapp_id = src->xapp_id};

  dst.len_e2_nodes_conn = src->len_e2_nodes_conn;

  if( dst.len_e2_nodes_conn > 0){
    dst.nodes = calloc( dst.len_e2_nodes_conn, sizeof(e2_node_connected_t) );
    assert(dst.nodes != NULL && "Memory exhausted");
  }

  for(size_t i = 0; i <  dst.len_e2_nodes_conn; ++i){
    dst.nodes[i] = cp_e2_node_connected(&src->nodes[i]);
  }

  return dst;
}

bool eq_e42_setup_response(const e42_setup_response_t* m0, const e42_setup_response_t* m1)
{
  if(m0 == m1)
    return true;

  if(m0 == NULL)
    return false;

  if(m1 == NULL)
    return false;

  if(m0->xapp_id != m1->xapp_id)
    return false;

  if(m0->len_e2_nodes_conn != m1->len_e2_nodes_conn)
    return false;

  for(size_t i = 0; i < m0->len_e2_nodes_conn; ++i){
    if( eq_e2_node_connected(&m0->nodes[i], &m1->nodes[i] ) == false)
      return false;
  }

  return true;
}

