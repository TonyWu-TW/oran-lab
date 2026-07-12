/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2ap_node_comp_conf_add_ack.h"
#include <assert.h>
#include <stdlib.h>

void free_e2ap_node_comp_conf_ack(e2ap_node_comp_conf_ack_t* src)
{
  assert(src != NULL);

  // Mandatory
  // Outcome
  //outcome_e2ap_node_comp_conf_ack_e outcome;

  // Optional
  // 9.2.1
  assert(src->cause == NULL && "Not implemented");
}


e2ap_node_comp_conf_ack_t cp_e2ap_node_comp_conf_ack( e2ap_node_comp_conf_ack_t const* src)
{
  assert(src != NULL);

  assert(0!=0 && "Not implemented");

  e2ap_node_comp_conf_ack_t dst = {0}; 
  return dst;
}

bool eq_e2ap_node_comp_conf_ack(e2ap_node_comp_conf_ack_t const* m0, e2ap_node_comp_conf_ack_t const* m1)
{
  if(m0 == m1)
    return true;
  if(m0 == NULL || m1 == NULL)
    return false;

  // Mandatory
  // Outcome
  if(m0->outcome != m1->outcome)
    return false;


  // Optional
  // 9.2.1
  assert(m0->cause == NULL && "Not implemented");
  assert(m1->cause == NULL && "Not implemented");

  return true;
}

