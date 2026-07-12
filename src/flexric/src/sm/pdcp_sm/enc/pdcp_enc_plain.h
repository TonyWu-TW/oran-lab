/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef PDCP_ENCODING_PLAIN_H
#define PDCP_ENCODING_PLAIN_H 

#include "../../../util/byte_array.h"
#include "../ie/pdcp_data_ie.h"


// Used for static polymorphism. 
// View pdcp_enc_generic file
typedef struct{

} pdcp_enc_plain_t;


byte_array_t pdcp_enc_event_trigger_plain(pdcp_event_trigger_t const* event_trigger);

byte_array_t pdcp_enc_action_def_plain(pdcp_action_def_t const*);

byte_array_t pdcp_enc_ind_hdr_plain(pdcp_ind_hdr_t const*); 

byte_array_t pdcp_enc_ind_msg_plain(pdcp_ind_msg_t const*); 

byte_array_t pdcp_enc_call_proc_id_plain(pdcp_call_proc_id_t const*); 

byte_array_t pdcp_enc_ctrl_hdr_plain(pdcp_ctrl_hdr_t const*); 

byte_array_t pdcp_enc_ctrl_msg_plain(pdcp_ctrl_msg_t const*); 

byte_array_t pdcp_enc_ctrl_out_plain(pdcp_ctrl_out_t const*); 

byte_array_t pdcp_enc_func_def_plain(pdcp_func_def_t const*);

#endif

