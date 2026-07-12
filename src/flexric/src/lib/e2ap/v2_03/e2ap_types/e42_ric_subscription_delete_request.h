/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E42_RIC_SUBSCRIPTION_DELETE_REQUEST_H
#define E42_RIC_SUBSCRIPTION_DELETE_REQUEST_H 

#include "ric_subscription_delete_request.h"
#include <stdint.h>

typedef struct{
 uint16_t xapp_id;
 ric_subscription_delete_request_t sdr;
} e42_ric_subscription_delete_request_t;


bool eq_e42_ric_subscription_delete_request(const e42_ric_subscription_delete_request_t* m0, const e42_ric_subscription_delete_request_t* m1);



#endif

