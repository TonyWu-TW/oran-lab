/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef GEN_MSG_VERSION_AGENT_MIR_H
#define GEN_MSG_VERSION_AGENT_MIR_H

#include "e2_agent.h"
#include "lib/e2ap/e2_setup_request_wrapper.h"

e2_setup_request_t gen_setup_request_v1(e2_agent_t* ag);

e2_setup_request_t gen_setup_request_v2(e2_agent_t* ag);

e2_setup_request_t gen_setup_request_v3(e2_agent_t* ag);


#define gen_setup_request(T,U) _Generic ((T), e2ap_v1_t*: gen_setup_request_v1, \
                                              e2ap_v2_t*: gen_setup_request_v2, \
                                             e2ap_v3_t*: gen_setup_request_v3, \
                                             default: gen_setup_request_v1) (U)

#endif
