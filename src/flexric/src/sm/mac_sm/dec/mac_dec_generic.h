/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */



#ifndef MAC_DECODING_GENERIC
#define MAC_DECODING_GENERIC 

#include "mac_dec_asn.h"
#include "mac_dec_fb.h"
#include "mac_dec_plain.h"

/////////////////////////////////////////////////////////////////////
// 9 Information Elements that are interpreted by the SM according
// to ORAN-WG3.E2SM-v01.00.00 Technical Specification
/////////////////////////////////////////////////////////////////////


#define mac_dec_event_trigger(T,U,V) _Generic ((T), \
                           mac_enc_plain_t*: mac_dec_event_trigger_plain, \
                           mac_enc_asn_t*: mac_dec_event_trigger_asn,\
                           mac_enc_fb_t*: mac_dec_event_trigger_fb,\
                           default: mac_dec_event_trigger_plain) (U,V)

#define mac_dec_action_def(T,U,V) _Generic ((T), \
                           mac_enc_plain_t*: mac_dec_action_def_plain, \
                           mac_enc_asn_t*: mac_dec_action_def_asn, \
                           mac_enc_fb_t*: mac_dec_action_def_fb, \
                           default:  mac_dec_action_def_plain) (U,V)

#define mac_dec_ind_hdr(T,U,V) _Generic ((T), \
                           mac_enc_plain_t*: mac_dec_ind_hdr_plain , \
                           mac_enc_asn_t*: mac_dec_ind_hdr_asn, \
                           mac_enc_fb_t*: mac_dec_ind_hdr_fb, \
                           default:  mac_dec_ind_hdr_plain) (U,V)

#define mac_dec_ind_msg(T,U,V) _Generic ((T), \
                           mac_enc_plain_t*: mac_dec_ind_msg_plain , \
                           mac_enc_asn_t*: mac_dec_ind_msg_asn, \
                           mac_enc_fb_t*: mac_dec_ind_msg_fb, \
                           default:  mac_dec_ind_msg_plain) (U,V)

#define mac_dec_call_proc_id(T,U,V) _Generic ((T), \
                           mac_enc_plain_t*: mac_dec_call_proc_id_plain , \
                           mac_enc_asn_t*: mac_dec_call_proc_id_asn, \
                           mac_enc_fb_t*: mac_dec_call_proc_id_fb, \
                           default:  mac_dec_call_proc_id_plain) (U,V)

#define mac_dec_ctrl_hdr(T,U,V) _Generic ((T), \
                           mac_enc_plain_t*: mac_dec_ctrl_hdr_plain , \
                           mac_enc_asn_t*: mac_dec_ctrl_hdr_asn, \
                           mac_enc_fb_t*: mac_dec_ctrl_hdr_fb, \
                           default: mac_dec_ctrl_hdr_plain) (U,V)

#define mac_dec_ctrl_msg(T,U,V) _Generic ((T), \
                           mac_enc_plain_t*: mac_dec_ctrl_msg_plain , \
                           mac_enc_asn_t*: mac_dec_ctrl_msg_asn, \
                           mac_enc_fb_t*: mac_dec_ctrl_msg_fb, \
                           default:  mac_dec_ctrl_msg_plain) (U,V)

#define mac_dec_ctrl_out(T,U,V) _Generic ((T), \
                           mac_enc_plain_t*: mac_dec_ctrl_out_plain , \
                           mac_enc_asn_t*: mac_dec_ctrl_out_asn, \
                           mac_enc_fb_t*: mac_dec_ctrl_out_fb, \
                           default:  mac_dec_ctrl_out_plain) (U,V)

#define mac_dec_func_def(T,U,V) _Generic ((T), \
                           mac_enc_plain_t*: mac_dec_func_def_plain, \
                           mac_enc_asn_t*: mac_dec_func_def_asn, \
                           mac_enc_fb_t*:  mac_dec_func_def_fb, \
                           default:  mac_dec_func_def_plain) (U,V)

#endif

