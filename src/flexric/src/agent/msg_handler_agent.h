/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef MSG_HANDLER_AGENT
#define MSG_HANDLER_AGENT

#include "e2_agent.h"

#include "lib/e2ap/type_defs_wrapper.h"

void init_handle_msg_agent(size_t len, handle_msg_fp_agent (*handle_msg)[len]);
//void init_handle_msg_agent(handle_msg_fp_agent (*handle_msg)[30]);

e2ap_msg_t e2ap_msg_handle_agent(e2_agent_t* agent, const e2ap_msg_t* msg);

///////////////////////////////////////////////////////////////////////////////////////////////////
// O-RAN E2APv01.01: Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// RIC -> E2
e2ap_msg_t e2ap_handle_subscription_request_agent(e2_agent_t* ag, const e2ap_msg_t* msg);

//RIC -> E2
e2ap_msg_t e2ap_handle_subscription_delete_request_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_control_request_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


///////////////////////////////////////////////////////////////////////////////////////////////////
// O-RAN E2APv01.01: Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// RIC <-> E2 
e2ap_msg_t e2ap_handle_error_indication_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_setup_response_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_setup_failure_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


// RIC <-> E2
e2ap_msg_t e2ap_handle_reset_request_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


// RIC <-> E2
e2ap_msg_t e2ap_handle_reset_response_agent(e2_agent_t* ag, const e2ap_msg_t* msg);

  
// RIC -> E2
e2ap_msg_t e2ap_handle_service_update_ack_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_service_update_failure_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_service_query_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_node_configuration_update_ack_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_node_configuration_update_failure_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_connection_update_agent(e2_agent_t* ag, const e2ap_msg_t* msg);


#endif

