/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "pending_event_ric.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int cmp_pending_event_ric(void const* p_v1, void const* p_v2)
{
  assert(p_v1 != NULL);
  assert(p_v2 != NULL);

  pending_event_ric_t* p1 = (pending_event_ric_t*)p_v1; 
  pending_event_ric_t* p2 = (pending_event_ric_t*)p_v2; 
  int cmp_ev = cmp_pending_event(p1, p2);
  if(cmp_ev != 0){ 
    return cmp_ev;
  }

  int rv = cmp_ric_gen_id(&p1->id, &p2->id);
  return rv;
}

bool eq_pending_event_ric(void const* p_v1, void const* p_v2)
{
  assert(p_v1 != NULL);
  assert(p_v2 != NULL);

  return cmp_pending_event_ric(p_v1, p_v2) == 0;
}

