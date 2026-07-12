/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_SUBSEQUENT_ACTION_H
#define RIC_SUBSEQUENT_ACTION_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  RIC_SUBS_REQ_CONTINUE = 0,
  RIC_SUBS_REQ_HALT = 1
} ric_subseq_action_type_t;

typedef struct ric_subsequent_action {
  ric_subseq_action_type_t type;
  uint32_t* time_to_wait_ms; // optional
} ric_subsequent_action_t;


bool eq_ric_subsequent_action(const ric_subsequent_action_t* m0, const ric_subsequent_action_t* m1);


#endif


