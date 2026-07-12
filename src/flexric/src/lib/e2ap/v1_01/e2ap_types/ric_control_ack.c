/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "ric_control_ack.h"


bool eq_ric_control_ack_req(const ric_control_acknowledge_t* m0, const ric_control_acknowledge_t* m1)
{
  if(m0 == m1) return true;

  if(m0 == NULL || m1 == NULL) return false;

  if(eq_ric_gen_id(&m0->ric_id, &m1->ric_id) == false)
    return false;

  if(eq_byte_array(m0->call_process_id, m1->call_process_id) == false)
    return false;

  if(m0->status != m1->status)
    return false;

  if(eq_byte_array(m0->control_outcome, m1->control_outcome)== false)
    return false;

  return true;
}
