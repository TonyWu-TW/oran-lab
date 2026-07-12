/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef MSG_HANDLER_RIC
#define MSG_HANDLER_RIC

#include "near_ric.h"
#include "../lib/e2ap/type_defs_wrapper.h"


e2ap_msg_t e2ap_msg_handle_ric(near_ric_t* ric, const e2ap_msg_t* msg);

///////////////////////////////////////////////////////////////////////////////////////////////////
// O-RAN E2APv01.01: Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// E2 -> RIC 
e2ap_msg_t e2ap_handle_subscription_response_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

//E2 -> RIC 
e2ap_msg_t e2ap_handle_subscription_failure_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

// E2 -> RIC
e2ap_msg_t e2ap_handle_subscription_delete_response_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

//E2 -> RIC
e2ap_msg_t e2ap_handle_subscription_delete_failure_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

// E2 -> RIC
e2ap_msg_t e2ap_handle_indication_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

// E2 -> RIC
e2ap_msg_t e2ap_handle_control_ack_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

// E2 -> RIC
e2ap_msg_t e2ap_handle_control_failure_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);
  
///////////////////////////////////////////////////////////////////////////////////////////////////
// O-RAN E2APv01.01: Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// RIC <-> E2 
e2ap_msg_t e2ap_handle_error_indication_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

// E2 -> RIC
e2ap_msg_t e2ap_handle_setup_request_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

// RIC <-> E2
e2ap_msg_t e2ap_handle_reset_request_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

// RIC <-> E2
e2ap_msg_t e2ap_handle_reset_response_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);
  
// E2 -> RIC
e2ap_msg_t e2ap_handle_service_update_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

// E2 -> RIC
e2ap_msg_t e2ap_handle_node_configuration_update_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

// E2 -> RIC
e2ap_msg_t e2ap_handle_connection_update_ack_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

// E2 -> RIC
e2ap_msg_t e2ap_handle_connection_update_failure_ric(struct near_ric_s* ric, const struct e2ap_msg_s* msg);

#endif

