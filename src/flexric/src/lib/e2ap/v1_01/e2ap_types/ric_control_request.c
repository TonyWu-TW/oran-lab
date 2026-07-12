/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "ric_control_request.h"

static
bool eq_ric_control_ack_req(const  ric_control_ack_req_t* m0, const  ric_control_ack_req_t* m1)
{
  if(m0 == m1) 
    return true;

  if(m0 == NULL || m1 == NULL)
    return false;

  if(*m0 != *m1)
    return false;

  return true;
}

bool eq_ric_control_request(const ric_control_request_t* m0, const ric_control_request_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(eq_ric_gen_id(&m0->ric_id, &m1->ric_id) == false)
    return false;

  if(eq_byte_array(m0->call_process_id, m1->call_process_id) == false)
    return false;

  if(eq_byte_array(&m0->hdr, &m1->hdr) == false)
    return false;

  if(eq_byte_array(&m0->msg, &m1->msg) == false)
    return false;

  if(eq_ric_control_ack_req(m0->ack_req, m1->ack_req) == false)
    return false;

  return true;
}

