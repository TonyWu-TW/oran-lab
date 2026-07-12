/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RLC_DECODING_FLATBUFFERS_H
#define RLC_DECODING_FLATBUFFERS_H

#include <stddef.h>
#include "../ie/rlc_data_ie.h"


rlc_event_trigger_t rlc_dec_event_trigger_fb(size_t len, uint8_t const ev_tr[len]);

rlc_action_def_t rlc_dec_action_def_fb(size_t len, uint8_t const action_def[len]);

rlc_ind_hdr_t rlc_dec_ind_hdr_fb(size_t len, uint8_t const ind_hdr[len]); 

rlc_ind_msg_t rlc_dec_ind_msg_fb(size_t len, uint8_t const ind_msg[len]); 

rlc_call_proc_id_t rlc_dec_call_proc_id_fb(size_t len, uint8_t const call_proc_id[len]);

rlc_ctrl_hdr_t rlc_dec_ctrl_hdr_fb(size_t len, uint8_t const ctrl_hdr[len]); 

rlc_ctrl_msg_t rlc_dec_ctrl_msg_fb(size_t len, uint8_t const ctrl_msg[len]); 

rlc_ctrl_out_t rlc_dec_ctrl_out_fb(size_t len, uint8_t const ctrl_out[len]); 

rlc_func_def_t rlc_dec_func_def_fb(size_t len, uint8_t const func_def[len]);

#endif

