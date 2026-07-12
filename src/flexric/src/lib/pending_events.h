/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef PENDING_EVENTS_H
#define PENDING_EVENTS_H

#include <stdbool.h>

typedef enum pending_event_e
{
  // AGENT
  SETUP_REQUEST_PENDING_EVENT,

  // RIC
  SUBSCRIPTION_REQUEST_PENDING_EVENT,
  SUBSCRIPTION_DELETE_REQUEST_PENDING_EVENT,
  CONTROL_REQUEST_PENDING_EVENT,

  // xApp
  E42_SETUP_REQUEST_PENDING_EVENT,
  E42_RIC_SUBSCRIPTION_REQUEST_PENDING_EVENT,
  RIC_SUBSCRIPTION_DELETE_REQUEST_PENDING_EVENT,
  E42_RIC_SUBSCRIPTION_DELETE_REQUEST_PENDING_EVENT,
  E42_RIC_CONTROL_REQUEST_PENDING_EVENT,  

} pending_event_t;


int cmp_pending_event(void const* pend_v1, void const* pend_v2);

bool valid_pending_event(pending_event_t ev);

#endif

