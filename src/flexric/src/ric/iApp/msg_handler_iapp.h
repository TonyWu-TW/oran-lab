/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef MSG_HANDLER_IAPP
#define MSG_HANDLER_IAPP

#include "e42_iapp.h"
#include "lib/e2ap/type_defs_wrapper.h"

void init_handle_msg_iapp(size_t len, handle_msg_fp_iapp (*handle_msg)[len]);

e2ap_msg_t e2ap_msg_handle_iapp(e42_iapp_t* iapp, const e2ap_msg_t* msg);


///////////////////////////////////////////////////////////////////////////////////////////////////
// O-RAN E2APv01.01: Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// RIC -> E2
e2ap_msg_t e2ap_handle_subscription_request_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);

//RIC -> E2
e2ap_msg_t e2ap_handle_subscription_delete_request_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_control_request_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);

// E2 -> RIC
e2ap_msg_t e2ap_handle_subscription_response_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


///////////////////////////////////////////////////////////////////////////////////////////////////
// O-RAN E2APv01.01: Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// RIC <-> E2 
e2ap_msg_t e2ap_handle_error_indication_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// xApp -> iApp
e2ap_msg_t e2ap_handle_e42_setup_request_iapp(e42_iapp_t* iapp, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_setup_failure_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// RIC <-> E2
e2ap_msg_t e2ap_handle_reset_request_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// RIC <-> E2
e2ap_msg_t e2ap_handle_reset_response_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);

  
// RIC -> E2
e2ap_msg_t e2ap_handle_service_update_ack_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_service_update_failure_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_service_query_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_node_configuration_update_ack_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_node_configuration_update_failure_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// RIC -> E2
e2ap_msg_t e2ap_handle_connection_update_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// xApp -> iApp
e2ap_msg_t e2ap_handle_e42_ric_subscription_request_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);

// xApp -> iApp
e2ap_msg_t e2ap_handle_e42_ric_subscription_delete_request_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);


// xApp -> iApp
e2ap_msg_t e2ap_handle_e42_ric_control_request_iapp(e42_iapp_t* ag, const e2ap_msg_t* msg);

// iApp -> xApp
e2ap_msg_t e2ap_handle_ric_indication_iapp(e42_iapp_t* iapp, const e2ap_msg_t* msg);

// iApp -> xApp
e2ap_msg_t e2ap_handle_subscription_delete_response_iapp( e42_iapp_t* iapp, const e2ap_msg_t* msg);



// iApp -> xApp 
e2ap_msg_t e2ap_handle_e42_ric_control_ack_iapp(e42_iapp_t* iapp, const e2ap_msg_t* msg);

// iApp -> xApp
e2ap_msg_t e2ap_handle_e42_ric_control_failure_iapp(e42_iapp_t* iapp, const e2ap_msg_t* msg);

#endif

