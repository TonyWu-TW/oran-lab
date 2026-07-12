/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef PDCP_DECODING_PLAIN_H
#define PDCP_DECODING_PLAIN_H

#include <stddef.h>
#include "../ie/pdcp_data_ie.h"


pdcp_event_trigger_t pdcp_dec_event_trigger_plain(size_t len, uint8_t const ev_tr[len]);

pdcp_action_def_t pdcp_dec_action_def_plain(size_t len, uint8_t const action_def[len]);

pdcp_ind_hdr_t pdcp_dec_ind_hdr_plain(size_t len, uint8_t const ind_hdr[len]); 

pdcp_ind_msg_t pdcp_dec_ind_msg_plain(size_t len, uint8_t const ind_msg[len]); 

pdcp_call_proc_id_t pdcp_dec_call_proc_id_plain(size_t len, uint8_t const call_proc_id[len]);

pdcp_ctrl_hdr_t pdcp_dec_ctrl_hdr_plain(size_t len, uint8_t const ctrl_hdr[len]); 

pdcp_ctrl_msg_t pdcp_dec_ctrl_msg_plain(size_t len, uint8_t const ctrl_msg[len]); 

pdcp_ctrl_out_t pdcp_dec_ctrl_out_plain(size_t len, uint8_t const ctrl_out[len]); 

pdcp_func_def_t pdcp_dec_func_def_plain(size_t len, uint8_t const func[len]);

#endif


