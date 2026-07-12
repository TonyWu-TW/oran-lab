/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_MSG_DEC_FB_H
#define E2AP_MSG_DEC_FB_H

#include "../type_defs.h"

typedef const struct e2ap_E2Message_table *e2ap_E2Message_table_t;
struct e2ap_fb;

e2ap_msg_t e2ap_msg_dec_fb(struct e2ap_fb* enc, byte_array_t ba);

void init_ap_fb(struct e2ap_fb*);

///////////////////////////////////////////////////////////////////////////////////////////////////
// O-RAN E2APv01.01: Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// RIC -> E2
e2ap_msg_t e2ap_dec_subscription_request_fb(e2ap_E2Message_table_t);

// E2 -> RIC 
e2ap_msg_t e2ap_dec_subscription_response_fb(e2ap_E2Message_table_t);

//E2 -> RIC 
e2ap_msg_t e2ap_dec_subscription_failure_fb(e2ap_E2Message_table_t);


//RIC -> E2
e2ap_msg_t e2ap_dec_subscription_delete_request_fb(e2ap_E2Message_table_t);


// E2 -> RIC
e2ap_msg_t e2ap_dec_subscription_delete_response_fb(e2ap_E2Message_table_t);


//E2 -> RIC
e2ap_msg_t e2ap_dec_subscription_delete_failure_fb(e2ap_E2Message_table_t);


// E2 -> RIC
e2ap_msg_t e2ap_dec_indication_fb(e2ap_E2Message_table_t);


// RIC -> E2
e2ap_msg_t e2ap_dec_control_request_fb( e2ap_E2Message_table_t);


// E2 -> RIC
e2ap_msg_t e2ap_dec_control_ack_fb(e2ap_E2Message_table_t);


// E2 -> RIC
e2ap_msg_t e2ap_dec_control_failure_fb(e2ap_E2Message_table_t);


///////////////////////////////////////////////////////////////////////////////////////////////////
// O-RAN E2APv01.01: Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// RIC <-> E2 
e2ap_msg_t e2ap_dec_error_indication_fb(e2ap_E2Message_table_t);


// E2 -> RIC
e2ap_msg_t e2ap_dec_setup_request_fb(e2ap_E2Message_table_t);


// RIC -> E2
e2ap_msg_t  e2ap_dec_setup_response_fb(e2ap_E2Message_table_t);


// RIC -> E2
e2ap_msg_t e2ap_dec_setup_failure_fb(e2ap_E2Message_table_t);


// RIC <-> E2
e2ap_msg_t e2ap_dec_reset_request_fb(e2ap_E2Message_table_t);


// RIC <-> E2
e2ap_msg_t e2ap_dec_reset_response_fb(e2ap_E2Message_table_t );

  
// E2 -> RIC
e2ap_msg_t e2ap_dec_service_update_fb(e2ap_E2Message_table_t);


// RIC -> E2
e2ap_msg_t e2ap_dec_service_update_ack_fb(e2ap_E2Message_table_t);


// RIC -> E2
e2ap_msg_t e2ap_dec_service_update_failure_fb(e2ap_E2Message_table_t);


// RIC -> E2
e2ap_msg_t e2ap_dec_service_query_fb(e2ap_E2Message_table_t);


// E2 -> RIC
e2ap_msg_t e2ap_dec_node_configuration_update_fb(e2ap_E2Message_table_t);


// RIC -> E2
e2ap_msg_t e2ap_dec_node_configuration_update_ack_fb(e2ap_E2Message_table_t);


// RIC -> E2
e2ap_msg_t e2ap_dec_node_configuration_update_failure_fb(e2ap_E2Message_table_t);


// RIC -> E2
e2ap_msg_t e2ap_dec_connection_update_fb(e2ap_E2Message_table_t);


// E2 -> RIC
e2ap_msg_t e2ap_dec_connection_update_ack_fb(e2ap_E2Message_table_t);


// E2 -> RIC
e2ap_msg_t e2ap_dec_connection_update_failure_fb(e2ap_E2Message_table_t);

#endif
