/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef GTP_ENCODING_PLAIN_H
#define GTP_ENCODING_PLAIN_H 

#include "../../../util/byte_array.h"
#include "../ie/gtp_data_ie.h"


// Used for static polymorphism. 
// View gtp_enc_generic file
typedef struct{

} gtp_enc_plain_t;


byte_array_t gtp_enc_event_trigger_plain(gtp_event_trigger_t const* event_trigger);

byte_array_t gtp_enc_action_def_plain(gtp_action_def_t const*);

byte_array_t gtp_enc_ind_hdr_plain(gtp_ind_hdr_t const*); 

byte_array_t gtp_enc_ind_msg_plain(gtp_ind_msg_t const*); 

byte_array_t gtp_enc_call_proc_id_plain(gtp_call_proc_id_t const*); 

byte_array_t gtp_enc_ctrl_hdr_plain(gtp_ctrl_hdr_t const*); 

byte_array_t gtp_enc_ctrl_msg_plain(gtp_ctrl_msg_t const*); 

byte_array_t gtp_enc_ctrl_out_plain(gtp_ctrl_out_t const*); 

byte_array_t gtp_enc_func_def_plain(gtp_func_def_t const*);

#endif

