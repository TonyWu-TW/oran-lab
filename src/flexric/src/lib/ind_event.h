/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef INDICATION_EVENT_H
#define INDICATION_EVENT_H

#include <stdint.h>                               // for uint8_t
#include "e2ap/ric_gen_id_wrapper.h"  // for ric_gen_id_t
#include "../sm/sm_agent.h"

typedef struct{
  ric_gen_id_t ric_id;
  // Non-owning ptr
  sm_agent_t* sm;
  uint8_t action_id;

  subscription_ans_e type;
  union {
  // Unknown type for the E2 Agent.
  // The RAN and the SMs know this type.
  // They will free it.
  // Periodic events may need this info
  void* act_def; // i.e., kpm_act_def_t 

  // Free function to call for aperiodic events
  void (*free_subs_aperiodic)(uint32_t ric_req_id);
  };

} ind_event_t;

int cmp_ind_event(void const* m0_v, void const* m1_v);

bool eq_ind_event_ric_req_id(const void* value, const void* key);

bool eq_ind_event(const void* value, const void* key);

// void free_ind_event(ind_event_t* src);

#endif

