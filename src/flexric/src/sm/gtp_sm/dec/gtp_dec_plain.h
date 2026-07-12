/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef GTP_DECODING_PLAIN_H
#define GTP_DECODING_PLAIN_H

#include <stddef.h>
#include "../ie/gtp_data_ie.h"


gtp_event_trigger_t gtp_dec_event_trigger_plain(size_t len, uint8_t const ev_tr[len]);

gtp_action_def_t gtp_dec_action_def_plain(size_t len, uint8_t const action_def[len]);

gtp_ind_hdr_t gtp_dec_ind_hdr_plain(size_t len, uint8_t const ind_hdr[len]); 

gtp_ind_msg_t gtp_dec_ind_msg_plain(size_t len, uint8_t const ind_msg[len]); 

gtp_call_proc_id_t gtp_dec_call_proc_id_plain(size_t len, uint8_t const call_proc_id[len]);

gtp_ctrl_hdr_t gtp_dec_ctrl_hdr_plain(size_t len, uint8_t const ctrl_hdr[len]); 

gtp_ctrl_msg_t gtp_dec_ctrl_msg_plain(size_t len, uint8_t const ctrl_msg[len]); 

gtp_ctrl_out_t gtp_dec_ctrl_out_plain(size_t len, uint8_t const ctrl_out[len]); 

gtp_func_def_t gtp_dec_func_def_plain(size_t len, uint8_t const func_def[len]);

#endif


