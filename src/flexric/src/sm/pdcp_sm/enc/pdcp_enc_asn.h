/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef PDCP_ENCRYPTIOIN_ASN_H
#define PDCP_ENCRYPTIOIN_ASN_H

#include "../../../util/byte_array.h"
#include "../ie/pdcp_data_ie.h"

// Used for static polymorphism. 
// See pdcp_enc_generic.h file
typedef struct{

} pdcp_enc_asn_t;

byte_array_t pdcp_enc_event_trigger_asn(pdcp_event_trigger_t const* event_trigger);

byte_array_t pdcp_enc_action_def_asn(pdcp_action_def_t const*);

byte_array_t pdcp_enc_ind_hdr_asn(pdcp_ind_hdr_t const*); 

byte_array_t pdcp_enc_ind_msg_asn(pdcp_ind_msg_t const*); 

byte_array_t pdcp_enc_call_proc_id_asn(pdcp_call_proc_id_t const*); 

byte_array_t pdcp_enc_ctrl_hdr_asn(pdcp_ctrl_hdr_t const*); 

byte_array_t pdcp_enc_ctrl_msg_asn(pdcp_ctrl_msg_t const*); 

byte_array_t pdcp_enc_ctrl_out_asn(pdcp_ctrl_out_t const*); 

byte_array_t pdcp_enc_func_def_asn(pdcp_func_def_t const*);

#endif

