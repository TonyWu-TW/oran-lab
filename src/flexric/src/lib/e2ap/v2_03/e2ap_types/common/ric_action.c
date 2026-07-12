/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "ric_action.h"

bool eq_ric_action(const ric_action_t* m0, const ric_action_t* m1)
{
  if(m0 == m1)
    return true;

  if(m0 == NULL || m1 == NULL)
    return false;

  if(m0->id != m1->id)
   return false;
 
  if(m0->type != m1->type)
    return false;

  if(eq_byte_array(m0->definition, m1->definition) == false)
    return false;

  if(eq_ric_subsequent_action(m0->subseq_action, m1->subseq_action) == false)
    return false;

  return true;
}

