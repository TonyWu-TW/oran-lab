/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e2ap_node_comp_id.h"
#include <assert.h>


static
bool supported_type(e2ap_node_comp_interface_type_e type){

  if(type == NG_E2AP_NODE_COMP_INTERFACE_TYPE 
     || type == F1_E2AP_NODE_COMP_INTERFACE_TYPE
     || type == E1_E2AP_NODE_COMP_INTERFACE_TYPE
     || type == S1_E2AP_NODE_COMP_INTERFACE_TYPE)
    return true;

  return false;
}


void free_e2ap_node_comp_id(e2ap_node_comp_id_t* src)
{
  assert(src != NULL);
  assert(supported_type(src->type) == true && "Only these types supported" );

  if(src->type == NG_E2AP_NODE_COMP_INTERFACE_TYPE  ){
    free_byte_array(src->ng_amf_name);
  } else if (src->type == F1_E2AP_NODE_COMP_INTERFACE_TYPE ){
  } else if (src->type == E1_E2AP_NODE_COMP_INTERFACE_TYPE ){
  } else if (src->type == S1_E2AP_NODE_COMP_INTERFACE_TYPE ){
    free_byte_array(src->s1_mme_name);
  } else {
    assert(0 != 0 && "Unknown type");
  }

}

e2ap_node_comp_id_t cp_e2ap_node_comp_id(e2ap_node_comp_id_t const* src)
{
  assert(src != NULL);
  assert(supported_type(src->type) == true && "Only these types supported" );

  e2ap_node_comp_id_t dst = {.type = src->type }; 

  if(src->type == NG_E2AP_NODE_COMP_INTERFACE_TYPE  ){
    dst.ng_amf_name = copy_byte_array(src->ng_amf_name); 
  } else if (src->type ==  F1_E2AP_NODE_COMP_INTERFACE_TYPE ){
    assert(src->f1_gnb_du_id < (1UL << 36));
    dst.f1_gnb_du_id = src->f1_gnb_du_id;
  } else if (src->type == E1_E2AP_NODE_COMP_INTERFACE_TYPE ){
    dst.e1_gnb_cu_up_id = src->e1_gnb_cu_up_id;
  } else if (src->type == S1_E2AP_NODE_COMP_INTERFACE_TYPE ){
    dst.s1_mme_name = copy_byte_array(src->s1_mme_name); 
  } else {
    assert(0 != 0 && "Unknown type");
  }

  return dst;
}

bool eq_e2ap_node_comp_id(e2ap_node_comp_id_t const* m0, e2ap_node_comp_id_t const* m1)
{
  if(m0 == m1)
    return true;

  if(m0 == NULL || m1 == NULL)
    return false;

  assert(supported_type(m0->type) == true && "Only these types supported" );
  assert(supported_type(m1->type) == true && "Only these types supported" );

  if(m0->type != m1->type)
    return false;

  if(m0->type == NG_E2AP_NODE_COMP_INTERFACE_TYPE  ){
    if(eq_byte_array(&m0->ng_amf_name, &m1->ng_amf_name) ==false)
      return false;
  } else if (m0->type == F1_E2AP_NODE_COMP_INTERFACE_TYPE ){
    assert(m0->f1_gnb_du_id < (1UL << 36));
    assert(m1->f1_gnb_du_id < (1UL << 36));
    if(m0->f1_gnb_du_id != m1->f1_gnb_du_id)
      return false;
  } else if (m0->type == E1_E2AP_NODE_COMP_INTERFACE_TYPE ){
    assert(m0->e1_gnb_cu_up_id < (1UL << 36));
    assert(m1->e1_gnb_cu_up_id < (1UL << 36));
    if(m0->e1_gnb_cu_up_id != m1->e1_gnb_cu_up_id)
      return false;
  } else if (m0->type == S1_E2AP_NODE_COMP_INTERFACE_TYPE ){
    if(eq_byte_array(&m0->s1_mme_name, &m1->s1_mme_name) ==false)
      return false;
  } else {
    assert(0 != 0 && "Unknown type");
  }

  return true;
}

