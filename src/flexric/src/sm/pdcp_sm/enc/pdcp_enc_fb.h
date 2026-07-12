/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef PDCP_ENCODING_FLATBUFFERS_H
#define PDCP_ENCODING_FLATBUFFERS_H

#include "../../../util/byte_array.h"
#include "../ie/pdcp_data_ie.h"


// Used for static polymorphism. 
// View pdcp_enc_generic file
typedef struct{

} pdcp_enc_fb_t;

byte_array_t pdcp_enc_event_trigger_fb(pdcp_event_trigger_t const* event_trigger);

byte_array_t pdcp_enc_action_def_fb(pdcp_action_def_t const*);

byte_array_t pdcp_enc_ind_hdr_fb(pdcp_ind_hdr_t const*); 

byte_array_t pdcp_enc_ind_msg_fb(pdcp_ind_msg_t const*); 

byte_array_t pdcp_enc_call_proc_id_fb(pdcp_call_proc_id_t const*); 

byte_array_t pdcp_enc_ctrl_hdr_fb(pdcp_ctrl_hdr_t const*); 

byte_array_t pdcp_enc_ctrl_msg_fb(pdcp_ctrl_msg_t const*); 

byte_array_t pdcp_enc_ctrl_out_fb(pdcp_ctrl_out_t const*); 

byte_array_t pdcp_enc_func_def_fb(pdcp_func_def_t const*);


#endif

