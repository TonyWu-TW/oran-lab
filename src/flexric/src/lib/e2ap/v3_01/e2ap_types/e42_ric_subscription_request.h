/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */




#ifndef E42_RIC_SUBSCRIPTION_REQUEST_H
#define E42_RIC_SUBSCRIPTION_REQUEST_H 

#include "common/e2ap_global_node_id.h"
#include "ric_subscription_request.h"

#include <stdbool.h>

typedef struct{
 uint16_t xapp_id;
 global_e2_node_id_t id;
 ric_subscription_request_t sr;
} e42_ric_subscription_request_t;

void free_e42_ric_subscription_request(e42_ric_subscription_request_t* sr);

bool eq_e42_ric_subscritption_request(const e42_ric_subscription_request_t* m0, const e42_ric_subscription_request_t* m1);

#endif

