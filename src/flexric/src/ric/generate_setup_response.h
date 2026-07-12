/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef GENERATE_SETUP_RESPONSE_MIR_H
#define GENERATE_SETUP_RESPONSE_MIR_H

#include "../lib/e2ap/e2_setup_response_wrapper.h"
#include "../lib/e2ap/e2_setup_request_wrapper.h"

typedef struct near_ric_s near_ric_t;

e2_setup_response_t generate_setup_response_v1(near_ric_t* ric, const e2_setup_request_t* req);

e2_setup_response_t generate_setup_response_v2(near_ric_t* ric, const e2_setup_request_t* req);

e2_setup_response_t generate_setup_response_v3(near_ric_t* ric, const e2_setup_request_t* req);


#define generate_setup_response(T,U,V) _Generic ((T), e2ap_v1_t*: generate_setup_response_v1, \
                                    e2ap_v2_t*: generate_setup_response_v2, \
                                    e2ap_v3_t*: generate_setup_response_v3, \
                                    default:  generate_setup_response_v1) (U,V)

#endif

