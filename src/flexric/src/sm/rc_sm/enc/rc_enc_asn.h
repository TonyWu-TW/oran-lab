/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RC_ENCRYPTIOIN_ASN_H
#define RC_ENCRYPTIOIN_ASN_H

#include "../../../util/byte_array.h"
#include "../ie/rc_data_ie.h"

// Used for static polymorphism. 
// See rc_enc_generic.h file
typedef struct{

} rc_enc_asn_t;

byte_array_t rc_enc_event_trigger_asn(e2sm_rc_event_trigger_t const* event_trigger);

byte_array_t rc_enc_action_def_asn(e2sm_rc_action_def_t const*);

byte_array_t rc_enc_ind_hdr_asn(e2sm_rc_ind_hdr_t const*); 

byte_array_t rc_enc_ind_msg_asn(e2sm_rc_ind_msg_t const*); 

byte_array_t rc_enc_cpid_asn(e2sm_rc_cpid_t const*); 

byte_array_t rc_enc_ctrl_hdr_asn(e2sm_rc_ctrl_hdr_t const*); 

byte_array_t rc_enc_ctrl_msg_asn(e2sm_rc_ctrl_msg_t const*); 

byte_array_t rc_enc_ctrl_out_asn(e2sm_rc_ctrl_out_t const*); 

byte_array_t rc_enc_func_def_asn(e2sm_rc_func_def_t const*);

#endif

