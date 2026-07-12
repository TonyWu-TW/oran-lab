/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */




#ifndef MESSAGE_GENERATOR_XAPP_H
#define MESSAGE_GENERATOR_XAPP_H 


#include "../lib/e2ap/ric_subscription_request_wrapper.h"
#include "../lib/e2ap/e42_setup_request_wrapper.h"
#include "../lib/e2ap/e42_ric_subscription_request_wrapper.h"

#include "e42_xapp.h"


ric_subscription_request_t generate_subscription_request(ric_gen_id_t ric_id, sm_ric_t const* sm, void* cmd);

e42_ric_subscription_request_t generate_e42_ric_subscription_request(uint16_t xapp_id, global_e2_node_id_t* id,  ric_subscription_request_t* sr); 

e42_setup_request_t generate_e42_setup_request(e42_xapp_t* xapp);

ric_control_request_t generate_ric_control_request(ric_gen_id_t ric_id, sm_ric_t const* sm, void* ctrl_msg);

//e42_ric_control_request_t generate_e42_ric_control_request(uint16_t xapp_id, global_e2_node_id_t* id,  ric_subscription_request_t* sr);

#endif

