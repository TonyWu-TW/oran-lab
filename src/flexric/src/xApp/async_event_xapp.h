/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef ASYNC_EVENT_XAPP_H
#define ASYNC_EVENT_XAPP_H 


#include "lib/pending_events.h"


typedef enum
{
  NETWORK_EVENT,
  INDICATION_EVENT,
  PENDING_EVENT,
  UNKNOWN_EVENT,
} async_event_xapp_e;

typedef struct{
  union{
    pending_event_t* p_ev;
//    ind_event_t* i_ev;
  };

 async_event_xapp_e type;
} async_event_xapp_t;

#endif

