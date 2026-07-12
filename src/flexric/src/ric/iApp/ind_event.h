/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef INDICATION_EVENT_H
#define INDICATION_EVENT_H

#include <stdint.h>                               // for uint8_t
#include "lib/ap/e2ap_types/common/ric_gen_id.h"  // for ric_gen_id_t
#include "sm/sm_agent.h"

typedef struct
{
  ric_gen_id_t ric_id;
  sm_iapp_t* sm;
  uint8_t action_id;
} ind_event_t;

int cmp_ind_event(void const* m0_v, void const* m1_v);

#endif

