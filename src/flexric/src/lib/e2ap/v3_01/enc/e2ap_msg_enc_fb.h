/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef E2AP_MSG_ENC_FB
#define E2AP_MSG_ENC_FB

#include "../type_defs.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Messages for Near-RT RIC Functional Procedures ///////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

byte_array_t e2ap_enc_subscription_request_fb(const ric_subscription_request_t* sr);
byte_array_t e2ap_enc_subscription_request_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_subscription_response_fb(const ric_subscription_response_t* sr);
byte_array_t e2ap_enc_subscription_response_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_subscription_failure_fb(const ric_subscription_failure_t* sf);
byte_array_t e2ap_enc_subscription_failure_fb_msg(const e2ap_msg_t* msg );

byte_array_t e2ap_enc_subscription_delete_request_fb(const ric_subscription_delete_request_t* sdr);
byte_array_t e2ap_enc_subscription_delete_request_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_subscription_delete_response_fb(const ric_subscription_delete_response_t* sdr);
byte_array_t e2ap_enc_subscription_delete_response_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_subscription_delete_failure_fb(const ric_subscription_delete_failure_t* sdf);
byte_array_t e2ap_enc_subscription_delete_failure_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_indication_fb(const ric_indication_t* ind);
byte_array_t e2ap_enc_indication_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_control_request_fb(const ric_control_request_t* cr);
byte_array_t e2ap_enc_control_request_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_control_ack_fb(const ric_control_acknowledge_t* ca);
byte_array_t e2ap_enc_control_ack_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_control_failure_fb(const ric_control_failure_t* cf);
byte_array_t e2ap_enc_control_failure_fb_msg(const e2ap_msg_t* msg);

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

byte_array_t e2ap_enc_error_indication_fb(const e2ap_error_indication_t* ei);
byte_array_t e2ap_enc_error_indication_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_setup_request_fb(const e2_setup_request_t* sr);
byte_array_t e2ap_enc_setup_request_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_setup_response_fb(const e2_setup_response_t* sr);
byte_array_t e2ap_enc_setup_response_fb_msg(const e2ap_msg_t* msg);

byte_array_t e2ap_enc_setup_failure_fb(const e2_setup_failure_t* sr);
byte_array_t e2ap_enc_setup_failure_msg(const e2ap_msg_t* msg);

//byte_array_t e2ap_enc_reset_request_fb(const e2ap_reset_request_t* rr);

//byte_array_t e2ap_enc_reset_response_fb(const e2ap_reset_response_t* rr);

//byte_array_t e2ap_enc_service_update_fb(const ric_service_update_t* su);

//byte_array_t e2ap_enc_service_update_ack_fb(const ric_service_update_ack_t* su);

//byte_array_t e2ap_enc_service_update_failure_fb(const ric_service_update_failure_t* uf);

//byte_array_t e2ap_enc_service_query_fb(const ric_service_query_t* sq);

//byte_array_t e2ap_enc_node_configuration_update_fb(const e2_node_configuration_update_t* cu);
//byte_array_t e2ap_enc_node_configuration_update_ack_fb(const e2_node_configuration_update_ack_t* cua);
//byte_array_t e2ap_enc_node_configuration_update_failure_fb(const e2_node_configuration_update_failure_t* cuf);

//byte_array_t e2ap_enc_node_connection_update_fb(const e2_node_connection_update_t* cu);

//byte_array_t e2ap_enc_node_connection_update_ack_fb(const e2_node_connection_update_ack_t* ca);

//byte_array_t e2ap_enc_node_connection_update_failure_fb(const e2_node_connection_update_failure_t*);

byte_array_t e2ap_enc_e42_setup_request_fb(const e42_setup_request_t* sr);
byte_array_t e2ap_enc_e42_setup_request_msg(const  e2ap_msg_t* msg);

byte_array_t e2ap_enc_e42_subscription_request_fb(const e42_setup_request_t* sr);
byte_array_t e2ap_enc_e42_subscription_request_msg(const  e2ap_msg_t* msg);

byte_array_t e2ap_enc_e42_subscription_delete_request_fb(const e42_setup_request_t* sr);
byte_array_t e2ap_enc_e42_subscription_delete_request_msg(const  e2ap_msg_t* msg);


byte_array_t e2ap_enc_e42_control_request_fb(const e42_ric_control_request_t* cr);
byte_array_t e2ap_enc_e42_control_request_msg(const  e2ap_msg_t* msg);


#endif
