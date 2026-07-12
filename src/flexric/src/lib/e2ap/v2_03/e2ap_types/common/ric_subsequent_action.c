/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "ric_subsequent_action.h"

#include <stddef.h>

bool eq_ric_subsequent_action(const ric_subsequent_action_t* m0, const ric_subsequent_action_t* m1)
{
  if(m0 == m1) 
    return true;

  if(m0 == NULL || m1 == NULL)
    return false;

  if(m0->type != m1->type) 
    return false;

  if(m0->time_to_wait_ms == m1->time_to_wait_ms)
    return true;

  if(m0->time_to_wait_ms == NULL || m1 == NULL)
    return false;

  if(*m0->time_to_wait_ms != *m1->time_to_wait_ms)
    return false;

  return true;

}
