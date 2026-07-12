/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RLC_ENCODING_PLAIN_H
#define RLC_ENCODING_PLAIN_H 

#include "../../../util/byte_array.h"
#include "../ie/rlc_data_ie.h"


// Used for static polymorphism. 
// View rlc_enc_generic file
typedef struct{

} rlc_enc_plain_t;


byte_array_t rlc_enc_event_trigger_plain(rlc_event_trigger_t const* event_trigger);

byte_array_t rlc_enc_action_def_plain(rlc_action_def_t const*);

byte_array_t rlc_enc_ind_hdr_plain(rlc_ind_hdr_t const*); 

byte_array_t rlc_enc_ind_msg_plain(rlc_ind_msg_t const*); 

byte_array_t rlc_enc_call_proc_id_plain(rlc_call_proc_id_t const*); 

byte_array_t rlc_enc_ctrl_hdr_plain(rlc_ctrl_hdr_t const*); 

byte_array_t rlc_enc_ctrl_msg_plain(rlc_ctrl_msg_t const*); 

byte_array_t rlc_enc_ctrl_out_plain(rlc_ctrl_out_t const*); 

byte_array_t rlc_enc_func_def_plain(rlc_func_def_t const*);

#endif

