/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#include "ric_action_admitted.h"

#include <assert.h>
#include <stddef.h>

bool eq_ric_action_admitted(const ric_action_admitted_t* m0, const ric_action_admitted_t* m1)
{
  if(m0 == m1 ) return true;

  if(m0 == NULL || m1 == NULL)
    return false;

  if(m0->ric_act_id != m1->ric_act_id)
    return false;

  return true;
}

ric_action_admitted_t cp_ric_action_admitted(ric_action_admitted_t const* src)
{
   assert(src != NULL);
   ric_action_admitted_t dst = {.ric_act_id = src->ric_act_id};
   return dst;
}

