/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_SUBSCRIPTION_REQUEST_H
#define RIC_SUBSCRIPTION_REQUEST_H

#include "util/byte_array.h"
#include "common/ric_gen_id.h"
#include "common/ric_action.h"
#include <stdbool.h>

typedef struct ric_subscription_request {
  ric_gen_id_t ric_id;
  byte_array_t event_trigger;
  ric_action_t* action;
  size_t len_action;
} ric_subscription_request_t;

bool eq_ric_subscritption_request(const ric_subscription_request_t* m0, const ric_subscription_request_t* m1);

// C++ move semantics
ric_subscription_request_t mv_ric_subscription_request( ric_subscription_request_t* sr);

#endif

