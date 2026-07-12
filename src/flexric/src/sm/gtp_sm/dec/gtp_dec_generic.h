/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef GTP_DECODING_GENERIC
#define GTP_DECODING_GENERIC 

#include "gtp_dec_plain.h"

/////////////////////////////////////////////////////////////////////
// 9 Information Elements that are interpreted by the SM according
// to ORAN-WG3.E2SM-v01.00.00 Technical Specification
/////////////////////////////////////////////////////////////////////


#define gtp_dec_event_trigger(T,U,V) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_dec_event_trigger_plain, \
                           default: gtp_dec_event_trigger_plain) (U,V)

#define gtp_dec_action_def(T,U,V) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_dec_action_def_plain, \
                           default:  gtp_dec_action_def_plain) (U,V)

#define gtp_dec_ind_hdr(T,U,V) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_dec_ind_hdr_plain , \
                           default:  gtp_dec_ind_hdr_plain) (U,V)

#define gtp_dec_ind_msg(T,U,V) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_dec_ind_msg_plain , \
                           default:  gtp_dec_ind_msg_plain) (U,V)

#define gtp_dec_call_proc_id(T,U,V) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_dec_call_proc_id_plain , \
                           default:  gtp_dec_call_proc_id_plain) (U,V)

#define gtp_dec_ctrl_hdr(T,U,V) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_dec_ctrl_hdr_plain , \
                           default: gtp_dec_ctrl_hdr_plain) (U,V)

#define gtp_dec_ctrl_msg(T,U,V) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_dec_ctrl_msg_plain , \
                           default:  gtp_dec_ctrl_msg_plain) (U,V)

#define gtp_dec_ctrl_out(T,U,V) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_dec_ctrl_out_plain , \
                           default:  gtp_dec_ctrl_out_plain) (U,V)

#define gtp_dec_func_def(T,U,V) _Generic ((T), \
                           gtp_enc_plain_t*: gtp_dec_func_def_plain, \
                           default:  gtp_dec_func_def_plain) (U,V)

#endif

