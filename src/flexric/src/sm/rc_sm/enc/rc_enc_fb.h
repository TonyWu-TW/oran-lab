/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef RC_ENCODING_FLATBUFFERS_H
#define RC_ENCODING_FLATBUFFERS_H

#include "../../../util/byte_array.h"
#include "../ie/rc_data_ie.h"


// Used for static polymorphism. 
// View rc_enc_generic file
typedef struct{

} rc_enc_fb_t;

byte_array_t rc_enc_event_trigger_fb(e2sm_rc_event_trigger_t const* event_trigger);

byte_array_t rc_enc_action_def_fb(e2sm_rc_action_def_t const*);

byte_array_t rc_enc_ind_hdr_fb(e2sm_rc_ind_hdr_t const*); 

byte_array_t rc_enc_ind_msg_fb(e2sm_rc_ind_msg_t const*); 

byte_array_t rc_enc_call_proc_id_fb(e2sm_rc_cpid_t const*); 

byte_array_t rc_enc_ctrl_hdr_fb(e2sm_rc_ctrl_hdr_t const*); 

byte_array_t rc_enc_ctrl_msg_fb(e2sm_rc_ctrl_msg_t const*); 

byte_array_t rc_enc_ctrl_out_fb(e2sm_rc_ctrl_out_t const*); 

byte_array_t rc_enc_func_def_fb(e2sm_rc_func_def_t const*);

#endif

