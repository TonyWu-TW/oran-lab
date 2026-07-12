/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_ACTION_ADMITTED_H
#define RIC_ACTION_ADMITTED_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t ric_act_id;
} ric_action_admitted_t;

bool eq_ric_action_admitted(const ric_action_admitted_t* m0, const ric_action_admitted_t* m1);

ric_action_admitted_t cp_ric_action_admitted(ric_action_admitted_t const* src);

#endif

