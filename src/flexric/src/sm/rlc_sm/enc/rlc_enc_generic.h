/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */


#ifndef RLC_ENCODING_GENERIC
#define RLC_ENCODING_GENERIC 

#include "rlc_enc_asn.h"
#include "rlc_enc_fb.h"
#include "rlc_enc_plain.h"

/////////////////////////////////////////////////////////////////////
// 9 Information Elements that are interpreted by the SM according
// to ORAN-WG3.E2SM-v01.00.00 Technical Specification
/////////////////////////////////////////////////////////////////////


#define rlc_enc_event_trigger(T,U) _Generic ((T), \
                           rlc_enc_plain_t*: rlc_enc_event_trigger_plain, \
                           rlc_enc_asn_t*: rlc_enc_event_trigger_asn,\
                           rlc_enc_fb_t*: rlc_enc_event_trigger_fb,\
                           default: rlc_enc_event_trigger_plain) (U)

#define rlc_enc_action_def(T,U) _Generic ((T), \
                           rlc_enc_plain_t*: rlc_enc_action_def_plain, \
                           rlc_enc_asn_t*: rlc_enc_action_def_asn, \
                           rlc_enc_fb_t*: rlc_enc_action_def_fb, \
                           default:  rlc_enc_action_def_plain) (U)

#define rlc_enc_ind_hdr(T,U) _Generic ((T), \
                           rlc_enc_plain_t*: rlc_enc_ind_hdr_plain , \
                           rlc_enc_asn_t*: rlc_enc_ind_hdr_asn, \
                           rlc_enc_fb_t*: rlc_enc_ind_hdr_fb, \
                           default:  rlc_enc_ind_hdr_plain) (U)

#define rlc_enc_ind_msg(T,U) _Generic ((T), \
                           rlc_enc_plain_t*: rlc_enc_ind_msg_plain , \
                           rlc_enc_asn_t*: rlc_enc_ind_msg_asn, \
                           rlc_enc_fb_t*: rlc_enc_ind_msg_fb, \
                           default:  rlc_enc_ind_msg_plain) (U)

#define rlc_enc_call_proc_id(T,U) _Generic ((T), \
                           rlc_enc_plain_t*: rlc_enc_call_proc_id_plain , \
                           rlc_enc_asn_t*: rlc_enc_call_proc_id_asn, \
                           rlc_enc_fb_t*: rlc_enc_call_proc_id_fb, \
                           default:  rlc_enc_call_proc_id_plain) (U)

#define rlc_enc_ctrl_hdr(T,U) _Generic ((T), \
                           rlc_enc_plain_t*: rlc_enc_ctrl_hdr_plain , \
                           rlc_enc_asn_t*: rlc_enc_ctrl_hdr_asn, \
                           rlc_enc_fb_t*: rlc_enc_ctrl_hdr_fb, \
                           default:  rlc_enc_ctrl_hdr_plain) (U)

#define rlc_enc_ctrl_msg(T,U) _Generic ((T), \
                           rlc_enc_plain_t*: rlc_enc_ctrl_msg_plain , \
                           rlc_enc_asn_t*: rlc_enc_ctrl_msg_asn, \
                           rlc_enc_fb_t*: rlc_enc_ctrl_msg_fb, \
                           default:  rlc_enc_ctrl_msg_plain) (U)

#define rlc_enc_ctrl_out(T,U) _Generic ((T), \
                           rlc_enc_plain_t*: rlc_enc_ctrl_out_plain , \
                           rlc_enc_asn_t*: rlc_enc_ctrl_out_asn, \
                           rlc_enc_fb_t*: rlc_enc_ctrl_out_fb, \
                           default:  rlc_enc_ctrl_out_plain) (U)

#define rlc_enc_func_def(T,U) _Generic ((T), \
                           rlc_enc_plain_t*: rlc_enc_func_def_plain, \
                           rlc_enc_asn_t*: rlc_enc_func_def_asn, \
                           rlc_enc_fb_t*:  rlc_enc_func_def_fb, \
                           default:  rlc_enc_func_def_plain) (U)

#endif

