/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2ap_node_comp_conf.h"
#include <assert.h>

void free_e2ap_node_comp_conf(e2ap_node_comp_conf_t* src)
{
  assert(src != NULL);

  free_byte_array(src->request); 
  free_byte_array(src->response); 
}

bool eq_e2ap_node_comp_conf(e2ap_node_comp_conf_t const* m0, e2ap_node_comp_conf_t const* m1)
{
  if(m0 == m1)
    return true;
  if(m0 == NULL || m1 == NULL)
    return false;

  if(eq_byte_array(&m0->request, &m1->request) == false)
    return false;

  if(eq_byte_array(&m0->response, &m1->response) == false)
    return false;

  return true;
}

e2ap_node_comp_conf_t cp_e2ap_node_comp_conf(e2ap_node_comp_conf_t const* src)
{
  assert(src != NULL);

  e2ap_node_comp_conf_t dst = {0};

  dst.request = copy_byte_array(src->request); 
  dst.response = copy_byte_array(src->response);  

  return dst;
}

