/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef E2AP_MSG_DEC_ASN_H
#define E2AP_MSG_DEC_ASN_H

#include "../type_defs.h"

struct E2AP_PDU;
typedef struct e2ap_asn e2ap_asn_t;

e2ap_msg_t e2ap_msg_dec_asn(e2ap_asn_t* asn, byte_array_t ba);

void e2ap_msg_free_asn(struct e2ap_asn* enc, e2ap_msg_t* msg);

void init_ap_asn(struct e2ap_asn*);

///////////////////////////////////////////////////////////////////////////////////////////////////
// O-RAN E2APv01.01: Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// RIC -> E2
e2ap_msg_t e2ap_dec_subscription_request(const struct E2AP_PDU* pdu);

// E2 -> RIC 
e2ap_msg_t e2ap_dec_subscription_response(const struct E2AP_PDU* pdu);

//E2 -> RIC 
e2ap_msg_t e2ap_dec_subscription_failure(const struct E2AP_PDU* pdu);


//RIC -> E2
e2ap_msg_t e2ap_dec_subscription_delete_request(const struct E2AP_PDU* pdu);


// E2 -> RIC
e2ap_msg_t e2ap_dec_subscription_delete_response(const struct E2AP_PDU* pdu);


//E2 -> RIC
e2ap_msg_t e2ap_dec_subscription_delete_failure(const struct E2AP_PDU* pdu);


// E2 -> RIC
e2ap_msg_t e2ap_dec_indication(const struct E2AP_PDU* pdu);


// RIC -> E2
e2ap_msg_t e2ap_dec_control_request(const struct E2AP_PDU* pdu);


// E2 -> RIC
e2ap_msg_t e2ap_dec_control_ack(const struct E2AP_PDU* pdu);


// E2 -> RIC
e2ap_msg_t e2ap_dec_control_failure(const struct E2AP_PDU* pdu);

  

///////////////////////////////////////////////////////////////////////////////////////////////////
// O-RAN E2APv01.01: Messages for Global Procedures ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// RIC <-> E2 
e2ap_msg_t e2ap_dec_error_indication(const struct E2AP_PDU* pdu);


// E2 -> RIC
e2ap_msg_t e2ap_dec_setup_request(const struct E2AP_PDU* pdu);


// RIC -> E2
e2ap_msg_t  e2ap_dec_setup_response(const struct E2AP_PDU* pdu);


// RIC -> E2
e2ap_msg_t e2ap_dec_setup_failure(const struct E2AP_PDU* pdu);


// RIC <-> E2
e2ap_msg_t e2ap_dec_reset_request(const struct E2AP_PDU* pdu);


// RIC <-> E2
e2ap_msg_t e2ap_dec_reset_response(const struct E2AP_PDU* pdu);

  

// E2 -> RIC
e2ap_msg_t e2ap_dec_service_update(const struct E2AP_PDU* pdu);


// RIC -> E2
e2ap_msg_t e2ap_dec_service_update_ack(const struct E2AP_PDU* pdu);


// RIC -> E2
 e2ap_msg_t e2ap_dec_service_update_failure(const struct E2AP_PDU* pdu);


// RIC -> E2
  e2ap_msg_t e2ap_dec_service_query(const struct E2AP_PDU* pdu);


// E2 -> RIC
e2ap_msg_t e2ap_dec_node_configuration_update(const struct E2AP_PDU* pdu);


// RIC -> E2
e2ap_msg_t e2ap_dec_node_configuration_update_ack(const struct E2AP_PDU* pdu);


// RIC -> E2
 e2ap_msg_t e2ap_dec_node_configuration_update_failure(const struct E2AP_PDU* pdu);


// RIC -> E2
 e2ap_msg_t e2ap_dec_connection_update(const struct E2AP_PDU* pdu);


// E2 -> RIC
 e2ap_msg_t e2ap_dec_connection_update_ack(const struct E2AP_PDU* pdu);


// E2 -> RIC
e2ap_msg_t e2ap_dec_connection_update_failure(const struct E2AP_PDU* pdu);

/////
// New in V2
/////

// E2 <-> RIC
e2ap_msg_t e2ap_dec_removal_request(const struct E2AP_PDU* pdu);

// E2 <-> RIC
e2ap_msg_t e2ap_dec_removal_response(const struct E2AP_PDU* pdu);

// E2 <-> RIC
e2ap_msg_t e2ap_dec_removal_failure(const struct E2AP_PDU* pdu);

/////
// End new in V2
/////


// xApp -> iApp
e2ap_msg_t e2ap_dec_e42_setup_request(const struct E2AP_PDU* pdu);

// iApp -> xApp
e2ap_msg_t e2ap_dec_e42_setup_response(const struct E2AP_PDU* pdu);

// xApp -> iApp 
e2ap_msg_t e2ap_dec_e42_subscription_request(const struct E2AP_PDU* pdu);

// xApp -> iApp 
e2ap_msg_t e2ap_dec_e42_subscription_delete_request(const struct E2AP_PDU* pdu);

// xApp -> iApp 
e2ap_msg_t e2ap_dec_e42_control_request(const struct E2AP_PDU* pdu);


#endif

