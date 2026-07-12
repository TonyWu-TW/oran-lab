/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef MAC_ENCODING_FLATBUFFERS_H
#define MAC_ENCODING_FLATBUFFERS_H

#include "../../../util/byte_array.h"
#include "../ie/mac_data_ie.h"


// Used for static polymorphism. 
// View mac_enc_generic file
typedef struct{

} mac_enc_fb_t;

byte_array_t mac_enc_event_trigger_fb(mac_event_trigger_t const* event_trigger);

byte_array_t mac_enc_action_def_fb(mac_action_def_t const*);

byte_array_t mac_enc_ind_hdr_fb(mac_ind_hdr_t const*); 

byte_array_t mac_enc_ind_msg_fb(mac_ind_msg_t const*); 

byte_array_t mac_enc_call_proc_id_fb(mac_call_proc_id_t const*); 

byte_array_t mac_enc_ctrl_hdr_fb(mac_ctrl_hdr_t const*); 

byte_array_t mac_enc_ctrl_msg_fb(mac_ctrl_msg_t const*); 

byte_array_t mac_enc_ctrl_out_fb(mac_ctrl_out_t const*); 

byte_array_t mac_enc_func_def_fb(mac_func_def_t const*);


#endif

