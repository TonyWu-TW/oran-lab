/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_ACTION_NOT_ADMITTED
#define  RIC_ACTION_NOT_ADMITTED

#include <stdbool.h>
#include <stdint.h>
#include "e2ap_cause.h"

typedef struct ric_action_not_admitted {
  uint8_t ric_act_id;
  cause_t cause;
} ric_action_not_admitted_t;

bool eq_ric_action_not_admitted(const   ric_action_not_admitted_t* m0, const   ric_action_not_admitted_t* m1);

ric_action_not_admitted_t cp_ric_action_not_admitted(ric_action_not_admitted_t* src);

#endif

