/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef APERIODIC_INDICATION_EVENT_MIR_H 
#define APERIODIC_INDICATION_EVENT_MIR_H

#include "e2ap/ric_gen_id_wrapper.h"
#include "../sm/sm_agent.h"
#include <stdint.h>

typedef struct{
  ric_gen_id_t ric_id;
  sm_agent_t* sm;
  uint8_t action_id;
  void* ind_data;
} aind_event_t;

typedef struct{
  size_t len;
  aind_event_t* arr;
} arr_aind_event_t;


#endif

