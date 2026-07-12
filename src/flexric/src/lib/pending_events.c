/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#include "pending_events.h"
#include <assert.h>
#include <stdlib.h>

int cmp_pending_event(void const* pend_v1, void const* pend_v2)
{
  assert(pend_v1 != NULL);
  assert(pend_v2 != NULL);

  pending_event_t* ev1 = (pending_event_t*)pend_v1; 
  pending_event_t* ev2 = (pending_event_t*)pend_v2; 

  if(*ev1 < *ev2) return 1;
  if(*ev1 == *ev2) return 0;
  return -1;
}


bool valid_pending_event(pending_event_t ev)
{
  assert(ev == SETUP_REQUEST_PENDING_EVENT
          || ev == SUBSCRIPTION_REQUEST_PENDING_EVENT
          || ev == SUBSCRIPTION_DELETE_REQUEST_PENDING_EVENT
          || ev == E42_SETUP_REQUEST_PENDING_EVENT
          );
  return true;
}

