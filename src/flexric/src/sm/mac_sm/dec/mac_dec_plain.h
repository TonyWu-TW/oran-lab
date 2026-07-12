/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef MAC_DECODING_PLAIN_H
#define MAC_DECODING_PLAIN_H

#include <stddef.h>
#include "../ie/mac_data_ie.h"


mac_event_trigger_t mac_dec_event_trigger_plain(size_t len, uint8_t const ev_tr[len]);

mac_action_def_t mac_dec_action_def_plain(size_t len, uint8_t const action_def[len]);

mac_ind_hdr_t mac_dec_ind_hdr_plain(size_t len, uint8_t const ind_hdr[len]); 

mac_ind_msg_t mac_dec_ind_msg_plain(size_t len, uint8_t const ind_msg[len]); 

mac_call_proc_id_t mac_dec_call_proc_id_plain(size_t len, uint8_t const call_proc_id[len]);

mac_ctrl_hdr_t mac_dec_ctrl_hdr_plain(size_t len, uint8_t const ctrl_hdr[len]); 

mac_ctrl_msg_t mac_dec_ctrl_msg_plain(size_t len, uint8_t const ctrl_msg[len]); 

mac_ctrl_out_t mac_dec_ctrl_out_plain(size_t len, uint8_t const ctrl_out[len]); 

mac_func_def_t mac_dec_func_def_plain(size_t len, uint8_t const func_def[len]);


#endif


