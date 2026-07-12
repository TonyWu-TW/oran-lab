/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RC_DECODING_FLATBUFFERS_H
#define RC_DECODING_FLATBUFFERS_H

#include <stddef.h>
#include "../ie/rc_data_ie.h"


e2sm_rc_event_trigger_t rc_dec_event_trigger_fb(size_t len, uint8_t const ev_tr[len]);

e2sm_rc_action_def_t rc_dec_action_def_fb(size_t len, uint8_t const action_def[len]);

e2sm_rc_ind_hdr_t rc_dec_ind_hdr_fb(size_t len, uint8_t const ind_hdr[len]); 

e2sm_rc_ind_msg_t rc_dec_ind_msg_fb(size_t len, uint8_t const ind_msg[len]); 

e2sm_rc_cpid_t rc_dec_call_proc_id_fb(size_t len, uint8_t const call_proc_id[len]);

e2sm_rc_ctrl_hdr_t rc_dec_ctrl_hdr_fb(size_t len, uint8_t const ctrl_hdr[len]); 

e2sm_rc_ctrl_msg_t rc_dec_ctrl_msg_fb(size_t len, uint8_t const ctrl_msg[len]); 

e2sm_rc_ctrl_out_t rc_dec_ctrl_out_fb(size_t len, uint8_t const ctrl_out[len]); 

e2sm_rc_func_def_t rc_dec_func_def_fb(size_t len, uint8_t const func_def[len]);

#endif

