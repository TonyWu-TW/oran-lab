/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef PENDING_EVENT_RIC_H
#define PENDING_EVENT_RIC_H

#include "pending_events.h"

#include <stdbool.h>
#include "e2ap/ric_gen_id_wrapper.h"

typedef struct
{
  pending_event_t ev;
  ric_gen_id_t id;
} pending_event_ric_t;  

int cmp_pending_event_ric(void const* p_v1, void const* p_v2);

bool eq_pending_event_ric(void const* p_v1, void const* p_v2); 

#endif

