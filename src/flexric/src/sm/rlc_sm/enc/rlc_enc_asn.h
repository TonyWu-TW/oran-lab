/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RLC_ENCRYPTIOIN_ASN_H
#define RLC_ENCRYPTIOIN_ASN_H

#include "../../../util/byte_array.h"
#include "../ie/rlc_data_ie.h"

// Used for static polymorphism. 
// See rlc_enc_generic.h file
typedef struct{

} rlc_enc_asn_t;

byte_array_t rlc_enc_event_trigger_asn(rlc_event_trigger_t const* event_trigger);

byte_array_t rlc_enc_action_def_asn(rlc_action_def_t const*);

byte_array_t rlc_enc_ind_hdr_asn(rlc_ind_hdr_t const*); 

byte_array_t rlc_enc_ind_msg_asn(rlc_ind_msg_t const*); 

byte_array_t rlc_enc_call_proc_id_asn(rlc_call_proc_id_t const*); 

byte_array_t rlc_enc_ctrl_hdr_asn(rlc_ctrl_hdr_t const*); 

byte_array_t rlc_enc_ctrl_msg_asn(rlc_ctrl_msg_t const*); 

byte_array_t rlc_enc_ctrl_out_asn(rlc_ctrl_out_t const*); 

byte_array_t rlc_enc_func_def_asn(rlc_func_def_t const*);

#endif

