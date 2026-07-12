/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_ACTION_H
#define RIC_ACTION_H

#include <stdbool.h>
#include <stdint.h>
#include "util/byte_array.h"
#include "ric_subsequent_action.h"

typedef enum { 
  RIC_ACT_REPORT = 0,
  RIC_ACT_INSERT = 1,
  RIC_ACT_POLICY = 2
} ric_action_type_t;


typedef struct ric_action {
  uint8_t id;
  ric_action_type_t type;
  byte_array_t* definition; // optional
  ric_subsequent_action_t* subseq_action; // optional
} ric_action_t;

bool eq_ric_action(const ric_action_t* m0, const ric_action_t* m1);

#endif
