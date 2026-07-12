/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_SUBSCRIPTION_DELETE_RESPONSE_H
#define RIC_SUBSCRIPTION_DELETE_RESPONSE_H

#include "common/ric_gen_id.h"
#include <stdbool.h>

typedef struct{
  ric_gen_id_t ric_id;
} ric_subscription_delete_response_t;

bool eq_ric_subscription_delete_response(const ric_subscription_delete_response_t* m0, const ric_subscription_delete_response_t* m1);

#endif

