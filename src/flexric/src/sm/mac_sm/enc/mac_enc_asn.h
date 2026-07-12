/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef MAC_ENCRYPTIOIN_ASN_H
#define MAC_ENCRYPTIOIN_ASN_H

#include "../../../util/byte_array.h"
#include "../ie/mac_data_ie.h"

// Used for static polymorphism. 
// See mac_enc_generic.h file
typedef struct{

} mac_enc_asn_t;

byte_array_t mac_enc_event_trigger_asn(mac_event_trigger_t const* event_trigger);

byte_array_t mac_enc_action_def_asn(mac_action_def_t const*);

byte_array_t mac_enc_ind_hdr_asn(mac_ind_hdr_t const*); 

byte_array_t mac_enc_ind_msg_asn(mac_ind_msg_t const*); 

byte_array_t mac_enc_call_proc_id_asn(mac_call_proc_id_t const*); 

byte_array_t mac_enc_ctrl_hdr_asn(mac_ctrl_hdr_t const*); 

byte_array_t mac_enc_ctrl_msg_asn(mac_ctrl_msg_t const*); 

byte_array_t mac_enc_ctrl_out_asn(mac_ctrl_out_t const*); 

byte_array_t mac_enc_func_def_asn(mac_func_def_t const*);

#endif

