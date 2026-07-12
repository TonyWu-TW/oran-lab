/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RIC_SUBSCRIPTION_DELETE_REQUEST_H
#define RIC_SUBSCRIPTION_DELETE_REQUEST_H

#include "common/ric_gen_id.h"
#include <stdbool.h>

typedef struct{
  ric_gen_id_t ric_id;
} ric_subscription_delete_request_t;

bool eq_ric_subscription_delete_request(const ric_subscription_delete_request_t* m0, const ric_subscription_delete_request_t* m1);

ric_subscription_delete_request_t cp_ric_subscription_delete_request(ric_subscription_delete_request_t const* src);

#endif

