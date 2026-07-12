/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RLC_ENCODING_FLATBUFFERS_H
#define RLC_ENCODING_FLATBUFFERS_H

#include "../../../util/byte_array.h"
#include "../ie/rlc_data_ie.h"


// Used for static polymorphism. 
// View rlc_enc_generic file
typedef struct{

} rlc_enc_fb_t;

byte_array_t rlc_enc_event_trigger_fb(rlc_event_trigger_t const* event_trigger);

byte_array_t rlc_enc_action_def_fb(rlc_action_def_t const*);

byte_array_t rlc_enc_ind_hdr_fb(rlc_ind_hdr_t const*); 

byte_array_t rlc_enc_ind_msg_fb(rlc_ind_msg_t const*); 

byte_array_t rlc_enc_call_proc_id_fb(rlc_call_proc_id_t const*); 

byte_array_t rlc_enc_ctrl_hdr_fb(rlc_ctrl_hdr_t const*); 

byte_array_t rlc_enc_ctrl_msg_fb(rlc_ctrl_msg_t const*); 

byte_array_t rlc_enc_ctrl_out_fb(rlc_ctrl_out_t const*); 

byte_array_t rlc_enc_func_def_fb(rlc_func_def_t const*);

#endif

