/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef ASYNC_EVENT_H
#define ASYNC_EVENT_H

#include "aind_event.h"
#include "ind_event.h"
#include "pending_events.h"
#include "ep/sctp_msg.h"

typedef enum
{
  CHECK_STOP_TOKEN_EVENT,
  SCTP_CONNECTION_SHUTDOWN_EVENT,
  SCTP_MSG_ARRIVED_EVENT, 
  INDICATION_EVENT,
  APERIODIC_INDICATION_EVENT,
  PENDING_EVENT,

  UNKNOWN_EVENT,
} async_event_e;

typedef struct
{
  async_event_e type;
  int fd;
  union{
    pending_event_t* p_ev;
    ind_event_t* i_ev;
    arr_aind_event_t ai_ev;
    sctp_msg_t msg; 
  };
} async_event_t;

typedef struct{
  async_event_t ev[64];
  int len;
} async_event_arr_t;

#endif

