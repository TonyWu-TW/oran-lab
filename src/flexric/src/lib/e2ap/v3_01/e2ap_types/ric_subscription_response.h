/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RIC_SUBSCRIPTION_RESPONSE_H
#define RIC_SUBSCRIPTION_RESPONSE_H 

#include <stddef.h>
#include "common/ric_gen_id.h"
#include "common/ric_action_admitted.h"
#include "common/ric_action_not_admitted.h"

#include <stdbool.h>

typedef struct {
  ric_gen_id_t ric_id;
  ric_action_admitted_t* admitted;
  size_t len_admitted;
  ric_action_not_admitted_t* not_admitted;
  size_t len_na;
} ric_subscription_response_t;

bool eq_ric_subscritption_response(const ric_subscription_response_t* m0, const ric_subscription_response_t* m1);

ric_subscription_response_t cp_ric_subscription_respponse(ric_subscription_response_t const* src);

// C++ move semantics
ric_subscription_response_t mv_ric_subscription_respponse(ric_subscription_response_t const* src);

#endif

